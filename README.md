# Midnight Graphics

## Functionality

### Window and Event Handling
A `Graphics::Window` represents an OS window and its surface. Currently, it is a single buffer: There is only one thread which blocks to wait for the previous frame to finish rendering. It's event handling is done with `window.pollEvent(event)` where `Graphics::Event event`. The event contains a variant which itself represents the current event off the event stack. There are various ways of handling this process nicely, though I prefer to make a functor:
```C++
struct EventHandler
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

    ...
};
```
which can be used with `std::visit(e_handler, event.event)` inside the poll event loop.

### Shader and Data
In order to actually draw things you first need the data you want to render and then the specification for *how* to draw that data. The former is a `Graphics::Buffer` and the latter your `Graphics::Pipeline`.

To create a buffer, the best way is to utilize the `Graphics::TypeBuffer`:
```C++
struct Vertex
{
	Math::Vec2f position;
	float rotation;
};

Graphics::TypeBuffer<Vertex> vertices;
vertices.resize(3);
vertices[0] = ...
...
```
To create a pipeline, we use a `Graphics::PipelineBuilder`. Check out the documentation for more info on that, but I always have preferred to use Lua scripts to create the pipeline. So it's as simple as 
```C++
auto pipeline = Graphics::PipelineBuilder(SOURCE_DIR, "/main.lua")
					.build()
```
Then, to draw we simply
```C++
auto rf = window.startFrame();

rf.clear({ 0.f, 0.f, 0.f });
rf.startRender();
rf.draw(pipeline, vertices);
rf.endRender();

window.endFrame(rf);
```