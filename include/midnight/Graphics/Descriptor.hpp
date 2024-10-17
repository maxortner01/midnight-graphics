#pragma once

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Image;

namespace Backend
{
    struct Sampler;
}

    struct DescriptorLayoutBuilder;
    
    // Basic abstraction of vulkan descriptor set
    // We should have pools built *out* as well, but for now each
    // set has its own pool (bad)
    struct Descriptor : ObjectHandle<Descriptor>
    {
        struct Layout : ObjectHandle<Layout>
        {
            struct Binding
            {
                enum Type
                {
                    Image, Sampler
                } type;

                uint32_t count;
            };

            Layout(Layout&& l);
            Layout(const Layout&) = delete;
            ~Layout();

            template<Binding::Type T>
            struct BindingData;

            bool hasVariableBinding() const { return variable_binding.has_value(); }
            const auto& getVariableBinding() const { return *variable_binding; }

            const auto& getBindings() const { return bindings; }

            friend struct DescriptorLayoutBuilder;

        private:
            Layout() = default;

            std::optional<Binding> variable_binding;
            std::vector<Binding> bindings;
        };

        struct Pool : ObjectHandle<Pool>, std::enable_shared_from_this<Pool>
        {
            ~Pool();

            Pool(const Pool&) = delete;
            Pool(Pool&&);

            std::shared_ptr<Descriptor> 
            allocateDescriptor(std::shared_ptr<Layout> layout);

            static std::shared_ptr<Pool> make()
            {
                return std::shared_ptr<Pool>(new Pool());
            }

        private:
            Pool();
        };

        Descriptor(const Descriptor&) = delete;
        Descriptor(Descriptor&&);

        ~Descriptor() = default;

        // If there's a 1:1 mapping of type -> index, we don't need to pass in index
        template<Layout::Binding::Type T>
        void update(uint32_t index, const typename Layout::BindingData<T>::Type& data);

        auto getLayoutHandle() const { return layout; }

        friend struct Pool;

    private:

        Descriptor() = default;

        // Binding index -> Descriptor::Binding::Type
        //std::unordered_map<uint32_t, Binding::Type> binding_types;

        std::shared_ptr<Layout> layout;
        std::shared_ptr<Pool>   pool;
    };

    template<>
    struct Descriptor::Layout::BindingData<Descriptor::Layout::Binding::Image>
    {
        using Type = std::vector<std::shared_ptr<Image>>;
    };

    template<>
    struct Descriptor::Layout::BindingData<Descriptor::Layout::Binding::Sampler>
    {
        using Type = std::vector<std::shared_ptr<Backend::Sampler>>;
    };

    struct DescriptorLayoutBuilder
    {
        MN_SYMBOL DescriptorLayoutBuilder& addBinding(Descriptor::Layout::Binding binding);
        MN_SYMBOL DescriptorLayoutBuilder& addVariableBinding(Descriptor::Layout::Binding::Type binding, uint32_t max_size);

        MN_SYMBOL [[nodiscard]] Descriptor::Layout build() const;

    private:
        std::optional<Descriptor::Layout::Binding> variable_binding;
        std::vector<Descriptor::Layout::Binding> bindings;
    };
}