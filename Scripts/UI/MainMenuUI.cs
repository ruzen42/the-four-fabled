using Godot;

public partial class MainMenuUI : CanvasLayer
{
	private Control _options;
	private Control _network;
	private Control _buttonsBox;
	public override void _Ready()
	{
		_options = GetNode<Control>("OptionsMenu");
		_network = GetNode<Control>("NetworkMenu");
		_buttonsBox = GetNode<Control>("ButtonsBox");
		_options.Connect("BackPressed", new Callable(this, nameof(OnOptionsBack)));
		_network.Connect("BackPressed", new Callable(this, nameof(OnNetworkBack)));
	}

	private void OnOptionsBack()
	{
		_options.Hide();
		_buttonsBox.Hide();
	}
	
	private void OnNetworkBack()
	{
		_network.Hide();
		_buttonsBox.Hide();
	}
}
