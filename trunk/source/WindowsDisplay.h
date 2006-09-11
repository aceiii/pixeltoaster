// Windows Display
// Copyright � 2004-2006, Glenn Fiedler
// http://www.pixeltoaster.com

class WindowsDisplay : public DisplayAdapter, public WindowsAdapter
{
public:

	WindowsDisplay()
	{
		direct3d = Direct3DCreate9( D3D_SDK_VERSION );
		defaults();
	}

	~WindowsDisplay()
	{
		close();

		if ( direct3d )
		{
			direct3d->Release();
			direct3d = NULL;
		}
	}

	bool open( const char title[], int width, int height, Output output, Mode mode )
	{
		DisplayAdapter::open( title, width, height, output, mode );

		window = new WindowsWindow( this, this, title, width, height );

		window->listener( DisplayAdapter::listener() );			// note: fixes bug where listener was forgotten after display close

		if ( !window->handle() )
		{
			close();
			return false;
		}

		bool result = false;

		if ( output==Output::Default || output == Output::Windowed )
			result = windowed();
		else
			result = fullscreen();

		if ( !result )
			return false;

		if ( DisplayAdapter::listener() )
			DisplayAdapter::listener()->onOpen( wrapper() ? *wrapper() : *(DisplayInterface*)this );

		return true;
	}

	void close()
	{
		delete device;
		delete window;

		DisplayAdapter::close();
	}

	bool update( const TrueColorPixel * trueColorPixels, const FloatingPointPixel * floatingPointPixels )
	{
		if ( shutdown )
		{
			close();
			return false;
		}

		if ( pendingToggle )
		{
			pendingToggle = false;

			if ( output() == Output::Windowed )
				fullscreen();
			else
				windowed();
		}

		if ( window )
			window->update();

		if ( device )
			device->update( trueColorPixels, floatingPointPixels );

		if ( window && !window->visible() )
		{
			window->show();
			window->update();
		}

		return true;
	}

	void listener( Listener * listener )
	{
		DisplayAdapter::listener( listener );

		if ( window )
			window->listener( listener );
	}

	// implement adapter interface for interoperability with window class

	bool paint()
	{
		if ( !device )
		{
			HDC dc = GetDC( window->handle() );

			if ( dc )
			{
				HBRUSH brush = CreateSolidBrush( RGB(0,0,0) );
				SelectObject( dc, brush );
				RECT rect;
				GetClientRect( window->handle(), &rect );
				Rectangle( dc, 0, 0, rect.right, rect.bottom );
				DeleteObject( brush );
				ReleaseDC( window->handle(), dc );
			}

			ValidateRect( window->handle(), NULL );

			return true;
		}
		else
		{
			return device->paint();
		}
	}

	bool fullscreen()
	{
		if ( device )
		{
			if ( output() == Output::Fullscreen )
				return true;

			delete device;
			device = NULL;
		}

		window->fullscreen( width(), height() );

		device = new WindowsDevice( direct3d, window->handle(), width(), height(), mode(), false );

		if ( !device->valid() )
		{
			close();
			return false;
		}

		DisplayAdapter::fullscreen();

		window->update();

		return true;
	}

	bool windowed()
	{
		if ( device )
		{
			if ( output() == Output::Windowed )
				return true;

			delete device;
			device = NULL;
		}

		window->windowed( width(), height() );

		device = new WindowsDevice( direct3d, window->handle(), width(), height(), mode(), true );

		if ( !device->valid() )
		{
			close();
			return false;
		}

		DisplayAdapter::windowed();

		return true;
	}

	void toggle()
	{
		pendingToggle = true;
	}

	void exit()
	{
		shutdown = true;
	}

protected:

	void defaults()
	{
		DisplayAdapter::defaults();
		window = NULL;
		device = NULL;
		shutdown = false;
		pendingToggle = false;
	}

private:

	LPDIRECT3D9 direct3d;
	WindowsWindow * window;
	WindowsDevice * device;
	bool shutdown;
	bool pendingToggle;
};
