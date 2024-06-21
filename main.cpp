
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
	mn::Graphics::Buffer::gpu_addr vertices;
};

int main()
{
	using namespace mn;

	Graphics::Window window(Math::Vec2u{ 1280U, 720U }, "Hello");
	EventVisitor v(window);

	auto pipeline = Graphics::PipelineBuilder::fromLua(SOURCE_DIR, "/main.lua")
		.setPushConstantObject<Constants>()
        .build();

	Graphics::TypeBuffer<Math::Vec2f> data;
	data.resize(3);
	data[0] = {  0.f,  1.f };
	data[1] = { -1.f, -1.f };
	data[2] = {  1.f, -1.f };

	while (!window.shouldClose())
	{
		Graphics::Event event;
		while (window.pollEvent(event))
			std::visit(v, event.event);

		auto rf = window.startFrame();

		rf.clear({ 0.25f, 0.f, 0.f });

		rf.startRender();
		rf.setPushConstant(pipeline, Constants{ data.getAddress() });
		rf.draw(pipeline, data.vertices());
		rf.endRender();

		window.endFrame(rf);
	}

	window.finishWork();
}