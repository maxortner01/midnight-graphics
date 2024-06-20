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
