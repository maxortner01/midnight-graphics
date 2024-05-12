#include <iostream>

#include <thread>
#include <numeric>
#include <memory>
#include <exception>
#include <cassert>
#include <vector>
#include <algorithm>
#include <span>

#include <vulkan/vulkan.h>
#include <SL/Lua.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

// https://stackoverflow.com/questions/54268425/enumerating-over-a-fold-expression
template<std::size_t... inds, class F>
constexpr void static_for_impl(std::index_sequence<inds...>, F&& f)
{
    (f(std::integral_constant<std::size_t, inds>{}), ...);
}

template<std::size_t N, class F>
constexpr void static_for(F&& f)
{
    static_for_impl(std::make_index_sequence<N>{}, std::forward<F>(f));
}

enum class Access
{
    ro = 0b01, wo = 0b10, rw = 0b11
};

struct Render
{
    // When the render is destroyed, we need to check if its corresponding
    // region is in the render list, in which case it needs to be destroyed
};

struct Storage
{
    Storage() : data(nullptr, 0)
    {   }

    std::pair<void*, std::size_t> data;
};

struct Region
{
    Region() : render(std::make_shared<Render>()), data(std::make_shared<Storage>())
    {   }

    bool isRendering() const
    {
        return (render.use_count() > 1);
    }

    // Copy region into this
    void copyInto(const std::unique_ptr<Region>& region)
    {
        if (data->data.second == region->getData()->data.second)
        {
            if (!data->data.second) 
                return;
        }
        else
        {
            if (data->data.first)
                std::free(data->data.first);
            
            data->data.second = region->getData()->data.second;
            data->data.first = std::calloc(data->data.second, 1);
        }
            
        std::memcpy(data->data.first, region->getData()->data.first, data->data.second);
    }

    std::shared_ptr<Render> startRender() const
    {
        return render;
    }
    
    std::shared_ptr<Storage> getData() const
    {
        return data;
    }

private:
    std::shared_ptr<Storage> data;
    std::shared_ptr<Render> render;
};

template<Access A>
struct Accessor
{
    Accessor(const Accessor&) = delete;

    Accessor(std::shared_ptr<Storage> s, std::shared_ptr<std::condition_variable> w) : storage(s), writing(w)
    { }

    void resize(std::size_t bytes)
    {
        if (bytes == storage->data.second) return;

        if (storage->data.first)
            std::free(storage->data.first);

        storage->data.first = std::calloc(bytes, 1);
        storage->data.second = bytes;
    }

    template<typename T>
    std::span<T> get()
    {
        assert(storage->data.second % sizeof(T) == 0);
        return { reinterpret_cast<T*>(storage->data.first), storage->data.second / sizeof(T) };
    }

    std::size_t size() const { return storage->data.second; }

    ~Accessor()
    {
        writing->notify_all();
    }

private:
    std::shared_ptr<Storage> storage;
    std::shared_ptr<std::condition_variable> writing;
};

template<>
struct Accessor<Access::ro>
{
    Accessor(const std::shared_ptr<Storage>& s) : storage(s)
    {   }

    template<typename T>
    std::span<const T> get()
    {
        assert(storage->data.second % sizeof(T) == 0);
        return { reinterpret_cast<const T*>(storage->data.first), storage->data.second / sizeof(T) };
    }

    std::size_t size() const { return storage->data.second; }

private:
    std::shared_ptr<Storage> storage;
};

template<typename ThreadEnum, ThreadEnum E>
using ThreadRegion = Region;

template<typename ThreadEnum, ThreadEnum... Threads>
struct RenderRegion
{
    RenderRegion() : 
        writing(std::make_shared<std::condition_variable>()),
        ul(m)
    {
        const auto threads = std::tuple(Threads...);
        static_for<sizeof...(Threads)>([this, threads](auto i)
        {
            constexpr std::size_t I = i;
            regions[std::get<I>(threads)] = std::make_unique<Region>();
        });
    }

    template<ThreadEnum L, Access A>
    std::unique_ptr<Accessor<A>> data()
    {
        // update current sync based on L and A
        // If it contains a write, we 
        //   (a) check if the region is rendering, in which case we move the region over to the rendering list
        //       and put a copy in its place
        //   (b) then, we put a lock on the region (which keeps any more write accessors from being created until
        //       this one goes out of scope

        if constexpr (A == Access::wo || A == Access::rw)
        {
            // We need to wait until there is no current write access
            wait_till_done_writing();

            // Then we need to check if the region is rendering
            if (regions.at(L)->isRendering())
            {
                std::cout << "region is rendering\n";
                // If it is, we move it to the rendering list
                rendering.emplace_back(std::move(regions.at(L)));
                regions.at(L) = std::make_unique<Region>();

                if (current_sync != L) { transfer<L>(); current_sync = L; }
                else regions.at(L)->copyInto(rendering.back());
                // Now we can set regions.at(L) pointer to a new region made from rendering.back()->makeCopy()
            }

            if (current_sync != L) { transfer<L>(); current_sync = L; }
            return std::make_unique<Accessor<A>>(regions.at(L)->getData(), writing);
        }
        else
        {
            // read only access is simple: if the current sync is not the thread, transfer
            if (current_sync != L) { transfer<L>(); }
            return std::make_unique<Accessor<A>>(regions.at(L)->getData());
        }
    }

    template<ThreadEnum L>
    std::shared_ptr<Render> render()
    {
        // Here we specify that a thread is rendering the data
        // If write access is requested while this particular render object
        // is alive, then the Region is migrated to a place where it is freed 
        // when Render dies and in its place a copy is put
        // otherwise, we can just keep it where it is (as no write access requests means the data isn't changed)
        wait_till_done_writing();
        return regions.at(L)->startRender();
    }

    std::size_t isolatedRegions() const
    {
        return rendering.size();
    }
    
    void flushIsolatedRegions()
    {
        flush.lock();
        auto it = std::find_if(rendering.begin(), rendering.end(), [](auto& r) { return !r->isRendering(); });
        while (it != rendering.end())
        {
            rendering.erase(it);
            it = std::find_if(rendering.begin(), rendering.end(), [](auto& r) { return !r->isRendering(); });
        }
        flush.unlock();
    }
    
    void wait_till_done_writing()
    {
        if (writing.use_count() > 1)
            writing->wait(ul);
    }

private:

    // Transfers the current sync region into the ThreadEnum region
    template<ThreadEnum L>
    void transfer()
    {
        if (current_sync == ThreadEnum::All) return;
        regions.at(L)->copyInto(regions.at(current_sync));
    }

    ThreadEnum current_sync = ThreadEnum::All;
    std::unordered_map<ThreadEnum, std::unique_ptr<Region>> regions;
    std::vector<std::unique_ptr<Region>> rendering;
    std::shared_ptr<std::condition_variable> writing;

    std::mutex m, flush;
    std::unique_lock<std::mutex> ul;
};

enum class THREADS
{
    A, B, All
};

const char* operator*(THREADS t)
{
    switch (t)
    {
    case THREADS::A: return "Thread A";
    case THREADS::B: return "Thread B";
    case THREADS::All: return "All Threads";
    }
}

template<THREADS Thread>
struct Base
{
    static void run(std::shared_ptr<RenderRegion<THREADS, THREADS::A, THREADS::B>> region)
    {
        using namespace std::chrono_literals;

        if constexpr (Thread == THREADS::A)
        {
            auto data = region->data<Thread, Access::rw>();
            data->resize(sizeof(float) * 4);
            auto span = data->template get<float>();
            for (uint32_t i = 0; i < span.size(); i++)
                span[i] = i;
        }

        uint32_t count = 0;

        while (true)
        {
            {
                std::cout << *Thread << ": computing" << std::endl;
                std::this_thread::sleep_for(0.5s);
            }
            {
                auto render = region->render<Thread>();
                std::cout << *Thread << ": rendering" << std::endl;
                auto data = region->data<Thread, Access::ro>()->template get<float>();
                for (const auto& f : data)
                    std::cout << "  " << f << "\n";

                if (rand() % 2 == 0)
                {
                    count++;
                    auto data = region->data<Thread, Access::rw>();
                    data->resize(sizeof(float) * 4);
                    auto span = data->template get<float>();
                    for (uint32_t i = 0; i < span.size(); i++)
                        span[i] = count * 4 + i;
                }

                std::this_thread::sleep_for(0.5s);
            }

            region->flushIsolatedRegions();
        }
    }
};

int main()
{
    using namespace std::chrono_literals;

    auto region = std::make_shared<RenderRegion<THREADS, THREADS::A, THREADS::B>>();
    std::thread threada(Base<THREADS::A>::run, region);

    std::this_thread::sleep_for(0.7s);
    std::thread threadb(Base<THREADS::B>::run, region);

    std::getchar();
    threada.join();
    threadb.join();
}

void run2()
{
    auto region = std::make_unique<RenderRegion<THREADS, THREADS::A>>();
    std::cout << region->isolatedRegions() << " isolated region(s)\n";

    {
        auto access = region->data<THREADS::A, Access::rw>();
        access->resize(sizeof(float));
        access->get<float>()[0] = 2.f;
    }
    {
        auto render = region->render<THREADS::A>();
        auto pre_render = region->data<THREADS::A, Access::ro>();
        std::cout << pre_render->get<float>()[0] << "\n";

        region->data<THREADS::A, Access::rw>()->get<float>()[0] = 4.f;

        std::cout << region->isolatedRegions() << " isolated region(s)\n";

        std::cout << region->data<THREADS::A, Access::ro>()->get<float>()[0] << "\n";
        std::cout << pre_render->get<float>()[0] << "\n";
        
        region->flushIsolatedRegions();
    }
    
    region->flushIsolatedRegions();

    std::cout << region->isolatedRegions() << " isolated region(s)\n";

    std::cout << "yo\n";
}
