#include <Graphics/Descriptor.hpp>
#include <Graphics/Backend/Instance.hpp>

#include <Graphics/Image.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics
{
    Descriptor::Descriptor(Descriptor&& d) :
        pool(d.pool),
        layout(d.layout)
    {
        handle = d.handle;
        d.pool = nullptr;
        d.layout = nullptr;
    }

    Descriptor::Layout::Layout(Layout&& l) :
        bindings(l.bindings),
        variable_binding(l.variable_binding)
    {
        std::swap(handle, l.handle);
    }

    Descriptor::Layout::~Layout()
    {
        auto& device = Backend::Instance::get()->getDevice();
        if (handle)
        {
            vkDestroyDescriptorSetLayout(
                device->getHandle().as<VkDevice>(),
                handle.as<VkDescriptorSetLayout>(),
                nullptr);
        }
    }

    template<>
    void Descriptor::update<Descriptor::Layout::Binding::Image>(uint32_t index, const std::vector<std::shared_ptr<Image>>& data)
    {
        auto& device = Backend::Instance::get()->getDevice();

        std::vector<VkDescriptorImageInfo> infos;
        for (const auto& image : data)
        {
            for (const auto& a : image->getColorAttachments())
                infos.push_back(VkDescriptorImageInfo{
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = static_cast<VkImageView>(a.view)
                });
        }

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = infos.size();
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.dstSet = static_cast<VkDescriptorSet>(handle);
        write.dstBinding = index;
        write.dstArrayElement = 0;
        write.pImageInfo = infos.data();

        vkUpdateDescriptorSets(device->getHandle().as<VkDevice>(), 1, &write, 0, nullptr);
    }

    template<>
    void Descriptor::update<Descriptor::Layout::Binding::Sampler>(uint32_t index, const std::vector<std::shared_ptr<Backend::Sampler>>& data)
    {
        auto& device = Backend::Instance::get()->getDevice();

        std::vector<VkDescriptorImageInfo> infos;
        infos.reserve(data.size());
        for (const auto& sampler : data)
        {
            infos.push_back(VkDescriptorImageInfo{
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .sampler = static_cast<VkSampler>(sampler->handle)
            });
        }

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = infos.size();
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        write.dstSet = static_cast<VkDescriptorSet>(handle);
        write.dstBinding = index;
        write.dstArrayElement = 0;
        write.pImageInfo = infos.data();

        vkUpdateDescriptorSets(device->getHandle().as<VkDevice>(), 1, &write, 0, nullptr);
    }

    DescriptorLayoutBuilder& 
    DescriptorLayoutBuilder::addBinding(Descriptor::Layout::Binding binding)
    {
        bindings.push_back(binding);
        return *this;
    }

    DescriptorLayoutBuilder& 
    DescriptorLayoutBuilder::addVariableBinding(Descriptor::Layout::Binding::Type type, uint32_t max_size)
    {
        variable_binding.emplace(Descriptor::Layout::Binding{ .type = type, .count = max_size });
        return *this;
    }

    VkDescriptorType get_type(Descriptor::Layout::Binding::Type t)
    {
        switch (t)
        {
        case Descriptor::Layout::Binding::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
        case Descriptor::Layout::Binding::Image:   return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        }
    }

    Descriptor::Layout DescriptorLayoutBuilder::build() const
    {
        std::vector<VkDescriptorType> types;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorBindingFlags> flags;

        for (uint32_t i = 0; i < this->bindings.size(); i++)
        {
            const auto type = get_type(this->bindings[i].type);

            types.push_back(type);

            bindings.push_back(VkDescriptorSetLayoutBinding{
                .binding = i,
                .descriptorCount = this->bindings[i].count,
                .descriptorType = type,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr
            });

            flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
        }

        // Variable descriptor here
        if (variable_binding)
        {
            const auto type = get_type(variable_binding->type);
            flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT);
            bindings.push_back(VkDescriptorSetLayoutBinding {
                .binding = static_cast<uint32_t>(this->bindings.size()),
                .descriptorCount = variable_binding->count,
                .descriptorType = get_type(variable_binding->type),
                .pImmutableSamplers = nullptr,
                .stageFlags = VK_SHADER_STAGE_ALL
            });
            types.push_back(type);
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
        bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlags.pNext = nullptr;
        bindingFlags.pBindingFlags = flags.data();
        bindingFlags.bindingCount = static_cast<uint32_t>(flags.size());

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createInfo.pBindings = bindings.data();
        createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

        // Set binding flags
        createInfo.pNext = &bindingFlags;

        auto& device = Backend::Instance::get()->getDevice();
        VkDescriptorSetLayout layout;
        MIDNIGHT_ASSERT(vkCreateDescriptorSetLayout(device->getHandle().as<VkDevice>(), &createInfo, nullptr, &layout)
            == VK_SUCCESS, "Failed to create descriptor set layout");
        
        Descriptor::Layout _layout;

        _layout.handle   = static_cast<mn::handle_t>(layout);
        _layout.bindings = this->bindings; 
        _layout.variable_binding = this->variable_binding;

        return _layout;
    }

    Descriptor::Pool::Pool()
    {
        // TODO: These values should be determined based off the physical device
        //       constraints
        std::unordered_map<VkDescriptorType, uint32_t> base_pool_sizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 4          },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 }
        };

        const std::vector<VkDescriptorPoolSize> pool_sizes = {
            VkDescriptorPoolSize{ .descriptorCount = 100, .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
            VkDescriptorPoolSize{ .descriptorCount = 4,   .type = VK_DESCRIPTOR_TYPE_SAMPLER }
        };

        VkDescriptorPoolCreateInfo pool_create_info{};
        pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_create_info.pNext = nullptr;
        pool_create_info.poolSizeCount = pool_sizes.size();
        pool_create_info.pPoolSizes = pool_sizes.data();
        pool_create_info.maxSets = 1;
        pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

        auto& device = Backend::Instance::get()->getDevice();

        VkDescriptorPool pool;
        MIDNIGHT_ASSERT(vkCreateDescriptorPool(
            device->getHandle().as<VkDevice>(), 
            &pool_create_info, 
            nullptr, 
            &pool) == VK_SUCCESS, "Failed to create descriptor pool");
        
        handle = static_cast<mn::handle_t>(pool);
    }

    Descriptor::Pool::Pool(Pool&& pool)
    {
        std::swap(handle, pool.handle);
    }

    Descriptor::Pool::~Pool()
    {
        auto& device = Backend::Instance::get()->getDevice();
        if (handle)
        {
            vkDestroyDescriptorPool(
                device->getHandle().as<VkDevice>(),
                handle.as<VkDescriptorPool>(),
                nullptr);
        }
    }

    std::shared_ptr<Descriptor> 
    Descriptor::Pool::allocateDescriptor(std::shared_ptr<Layout> layout)
    {
        assert(layout.get());
        std::vector<VkDescriptorType> types;
        for (const auto& binding : layout->getBindings())
            types.push_back(get_type(binding.type));

        auto shared_pool = shared_from_this();

        // Build the descriptor set
        const auto set = [&layout, &shared_pool]() -> mn::handle_t
        {
            VkDescriptorSetVariableDescriptorCountAllocateInfo variable_alloc{};
            uint32_t count = ( layout->hasVariableBinding() ? layout->getVariableBinding().count : 0U );
            if (layout->hasVariableBinding())
            {
                variable_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                variable_alloc.pNext = nullptr;
                variable_alloc.descriptorSetCount = 1;
                variable_alloc.pDescriptorCounts = &count;
            }

            const auto desc_layout = layout->getHandle().as<VkDescriptorSetLayout>();
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.pNext = ( layout->hasVariableBinding() ? &variable_alloc : nullptr );
            alloc_info.descriptorPool = shared_pool->getHandle().as<VkDescriptorPool>();
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &desc_layout;

            auto& device = Backend::Instance::get()->getDevice();

            VkDescriptorSet set;
            MIDNIGHT_ASSERT(vkAllocateDescriptorSets(
                device->getHandle().as<VkDevice>(), 
                &alloc_info, 
                &set) == VK_SUCCESS, "Failed to create set");

            return static_cast<mn::handle_t>(set);
        }();

        auto d = std::shared_ptr<Descriptor>(new Descriptor());

        d->handle = set;
        d->pool   = shared_pool;
        d->layout = layout;

        return d;
    }
}