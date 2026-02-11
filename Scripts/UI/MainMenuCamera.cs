using Godot;

namespace TheFourFabled.Scripts.UI;

public partial class MainMenuCamera : Camera2D
{
	public override void _Process(double delta) => Position = GetGlobalMousePosition();
}