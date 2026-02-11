using Godot;
public partial class MainMenuButtons : VBoxContainer
{
	private PackedScene scene;
	public override void _Ready()
	{
		scene = GD.Load<PackedScene>("res://Scenes/main_game.tscn");
	}

	public override void _Process(double delta)
	{
	}

	public void OnOptionsButton_Clicked()
	{
		Visible = false;
	}

	public void OnExitButton_Clicked() => GetTree().Quit();

	public void OnPlayButton_Clicked()
	{
		GD.Print("Game Started");
		GetTree().ChangeSceneToPacked(scene);
	}
}
