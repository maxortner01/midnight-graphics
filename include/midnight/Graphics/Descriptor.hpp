#pragma once

#include "ObjectHandle.hpp"

namespace mn::Graphics
{
    struct Image;

namespace Backend
{
    struct Sampler;
}

    struct DescriptorBuilder;

    // Basic abstraction of vulkan descriptor set
    // We should have pools built *out* as well, but for now each
    // set has its own pool (bad)
    struct Descriptor : ObjectHandle<Descriptor>
    {
        struct Binding
        {
            enum Type
            {
                Image, Sampler
            } type;

            uint32_t count;
        };

        template<Binding::Type T>
        struct BindingData;
        
        Descriptor(const Descriptor&) = delete;
        Descriptor(Descriptor&&);

        ~Descriptor();

        // If there's a 1:1 mapping of type -> index, we don't need to pass in index
        template<Binding::Type T>
        void update(uint32_t index, const typename BindingData<T>::Type& data);

        auto getLayoutHandle() const { return layout; }

    private:
        friend struct DescriptorBuilder;

        Descriptor() = default;

        // Binding index -> Descriptor::Binding::Type
        //std::unordered_map<uint32_t, Binding::Type> binding_types;
        mn::handle_t pool, layout;
    };

    template<>
    struct Descriptor::BindingData<Descriptor::Binding::Image>
    {
        using Type = std::vector<std::shared_ptr<Image>>;
    };

    template<>
    struct Descriptor::BindingData<Descriptor::Binding::Sampler>
    {
        using Type = std::vector<std::shared_ptr<Backend::Sampler>>;
    };

    struct DescriptorBuilder
    {
        MN_SYMBOL DescriptorBuilder& addBinding(Descriptor::Binding binding);
        MN_SYMBOL DescriptorBuilder& addVariableBinding(Descriptor::Binding::Type type, uint32_t max_size);

        MN_SYMBOL [[nodiscard]] Descriptor build() const;

    private:
        std::optional<Descriptor::Binding> variable_binding;
        std::vector<Descriptor::Binding> bindings;
    };
}