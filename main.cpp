
#include <midnight/midnight.hpp>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

struct EventVisitor
{
	mn::Graphics::Window& window;

	EventVisitor(mn::Graphics::Window& w) :
		window(w)
	{	}

	template<typename T>
	void operator() (T value) const
	{	}

	void operator() (mn::Graphics::Event::Quit)
	{
		window.close();
	}
};

struct Constants
{
	mn::Graphics::Buffer::gpu_addr models;
};

int main()
{
	using namespace mn;

	Graphics::Window window(Math::Vec2u{ 1280U, 720U }, "Hello");
	EventVisitor v(window);

	auto pipeline = Graphics::PipelineBuilder::fromLua(SOURCE_DIR, "/main.lua")
		.setPushConstantObject<Constants>()
        .build();

	auto model = Graphics::Model::fromFrame([]()
	{
		Graphics::Model::Frame frame;
		frame.vertices = {
			{ { -1.f,  1.f, -5.5f } },
			{ { -1.f, -1.f, -5.5f } },
			{ {  1.f, -1.f, -5.5f } },
			{ {  1.f,  1.f, -5.5f } }
		};

		frame.indices = { 0, 1, 2, 2, 3, 0 };

		return frame;
	}());

	Graphics::TypeBuffer<Math::Mat4<float>> model_mats;
	model_mats.resize(2);

	uint32_t i = 0;
	while (!window.shouldClose())
	{
		Graphics::Event event;
		while (window.pollEvent(event))
			std::visit(v, event.event);

		auto rf = window.startFrame();

		rf.clear({ 0.25f, 0.f, 0.f });

		Constants c;
		c.models = model_mats.getAddress();
		model_mats[0] = Math::perspective<float>(window.aspectRatio(), Math::Angle::degrees(50), { 0.1f, 100.f });
		model_mats[1] = Math::rotation<float>(Math::Vec3<Math::Angle>( {
			Math::Angle::radians(0), 
			Math::Angle::radians(0), 
			Math::Angle::radians(i / 100.f)
		} ));

		rf.startRender();
		rf.setPushConstant(pipeline, c);
		rf.draw(pipeline, model);
		rf.endRender();

		window.endFrame(rf);
		i++;
	}

	window.finishWork();
}