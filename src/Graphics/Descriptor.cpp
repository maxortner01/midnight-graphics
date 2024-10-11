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

    Descriptor::~Descriptor()
    {
        auto& device = Backend::Instance::get()->getDevice();
        if (pool)
        {
            vkDestroyDescriptorPool(
                device->getHandle().as<VkDevice>(),
                static_cast<VkDescriptorPool>(pool),
                nullptr);
        }

        if (layout)
        {
            vkDestroyDescriptorSetLayout(
                device->getHandle().as<VkDevice>(),
                static_cast<VkDescriptorSetLayout>(layout),
                nullptr);
        }
    }

    template<>
    void Descriptor::update<Descriptor::Binding::Image>(uint32_t index, const std::vector<std::shared_ptr<Image>>& data)
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
    void Descriptor::update<Descriptor::Binding::Sampler>(uint32_t index, const std::vector<std::shared_ptr<Backend::Sampler>>& data)
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

    DescriptorBuilder& 
    DescriptorBuilder::addBinding(Descriptor::Binding binding)
    {
        bindings.push_back(binding);
        return *this;
    }

    DescriptorBuilder& 
    DescriptorBuilder::addVariableBinding(Descriptor::Binding::Type type, uint32_t max_size)
    {
        variable_binding.emplace(Descriptor::Binding{ .type = type, .count = max_size });
        return *this;
    }

    Descriptor DescriptorBuilder::build() const
    {
        // Build the layout
        std::vector<VkDescriptorType> types;
        const auto layout = [this, &types]() -> mn::handle_t
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            std::vector<VkDescriptorBindingFlags> flags;

            const auto get_type = [](Descriptor::Binding::Type t)
            {
                switch (t)
                {
                case Descriptor::Binding::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
                case Descriptor::Binding::Image:   return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                }
            };

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
            return static_cast<mn::handle_t>(layout);
        }();

        // Build the pool
        const auto pool = [this, &types]() -> mn::handle_t
        {   
            // TODO: These values should be determined based off the physical device
            //       constraints
            std::unordered_map<VkDescriptorType, uint32_t> base_pool_sizes = {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 4          },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 }
            };

            std::vector<VkDescriptorPoolSize> pool_sizes;
            for (const auto& type : types)
                pool_sizes.push_back(VkDescriptorPoolSize {
                    .descriptorCount = base_pool_sizes.at(type),
                    .type = type
                });

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

            return static_cast<mn::handle_t>(pool);
        }();

        // Build the descriptor set
        const auto set = [this, pool, layout]() -> mn::handle_t
        {
            VkDescriptorSetVariableDescriptorCountAllocateInfo variable_alloc{};
            uint32_t count = ( variable_binding ? variable_binding->count : 0U );
            if (variable_binding)
            {
                variable_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                variable_alloc.pNext = nullptr;
                variable_alloc.descriptorSetCount = 1;
                variable_alloc.pDescriptorCounts = &count;
            }

            const auto desc_layout = static_cast<VkDescriptorSetLayout>(layout);
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.pNext = ( variable_binding ? &variable_alloc : nullptr );
            alloc_info.descriptorPool = static_cast<VkDescriptorPool>(pool);
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

        Descriptor d;

        d.handle = set;
        d.pool   = pool;
        d.layout = layout;

        return d;
    }
}