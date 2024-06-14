
#include <midnight/midnight.hpp>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;/**/

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

int main()
{
	using namespace mn;

	Graphics::Image image(109, { 1280U, 720U }, false);

	/*
	Graphics::Window window({ 1280U, 720U }, "Hello");
	EventVisitor v(window);

	while (!window.shouldClose())
	{
		Graphics::Event event;
		while (window.pollEvent(event))
			std::visit(v, event.event);

		auto rf = window.startFrame();
		window.endFrame(rf);
	}*/
}