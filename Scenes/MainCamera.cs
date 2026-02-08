using Godot;
using System;

public partial class MainCamera : Camera2D
{
	public override void _Process(double _)
	{
		// Camera follow mouse every frame
		Position = GetViewport().GetMousePosition();
	}
}
