using Godot;
public partial class MainMenuButtons : VBoxContainer
{
	private PackedScene scene;
	public override void _Ready() => scene = GD.Load<PackedScene>("res://Scenes/main_game.tscn");
	[Export] public Control OptionsMenu;
	[Export] public Control NetworkMenu;

	public void OnOptionsButton_Clicked()
	{
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
		GD.Print("[UI] Game Started");
		GetTree().ChangeSceneToPacked(scene);
	}

	public void NetworkButton_OnClick()
	{
		GD.Print("[UI] Networks menu opened");
		NetworkMenu.Show();
	}
}
