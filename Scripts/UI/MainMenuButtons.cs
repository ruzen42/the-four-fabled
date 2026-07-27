using Godot;
public partial class MainMenuButtons : VBoxContainer
{
	private PackedScene scene;
	[Export] public Control OptionsMenu;
	[Export] public Control NetworkMenu;
	[Export] public AudioStreamPlayer _player;
	public override void _Ready() => scene = GD.Load<PackedScene>("res://Scenes/main_game.tscn");

	public void OnOptionsButton_Clicked()
	{
		_player.Play();
		GD.Print("[UI] Options opened");
		OptionsMenu.Show();
	}

	public void OnExitButton_Clicked()
	{
		GD.Print("[MAIN] Exiting Game");
		GetTree().Quit();
	}


	public void OnSingleplayerButton_Click()
	{
		_player.Play();
		GD.Print("[UI] Game Started");
		GetTree().ChangeSceneToPacked(scene);
	}

	public void NetworkButton_OnClick()
	{
		_player.Play();
		GD.Print("[UI] Networks menu opened");
		NetworkMenu.Show();
	}
}
