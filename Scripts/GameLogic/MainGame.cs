using Godot;
using System;
using TheFourFabled.Scripts.AutoLoad;

public partial class MainGame : CanvasLayer
{
	private SceneTree _sceneTree;
	[Export] public TheFourFabled.Scripts.UI.PauseMenu PauseMenu;

	public override void _Ready() => _sceneTree = GetTree();

	public override void _Input(InputEvent @event)
	{
		if (!@event.IsActionPressed("exit") || !Visible) return;
		//_sceneTree.Paused = true;
		PauseMenu.Show();
		
	}
	
	
}
