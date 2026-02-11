using Godot;
public partial class MainMenuButtons : VBoxContainer
{
	private PackedScene scene;
	public override void _Ready() => scene = GD.Load<PackedScene>("res://Scenes/main_game.tscn");


	public void OnOptionsButton_Clicked()
	{
		GD.Print("Options opened");
		Visible = false;
	}

	public void OnExitButton_Clicked()
	{
		GD.Print("Exiting Game");
		GetTree().Quit();
	}


	public void OnSingleplayerButton_Click()
	{
		GD.Print("Game Started");
		GetTree().ChangeSceneToPacked(scene);
	}
}
