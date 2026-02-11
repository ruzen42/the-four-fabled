using Godot;
using TheFourFabled.Scripts.AutoLoad;

namespace TheFourFabled.Scripts.UI;

public partial class PauseMenu : Control
{
	private SceneTree _sceneTree = null!;

	public override void _Ready()
	{
		Visible = false;
		_sceneTree = GetTree();
		GetNode<Button>("PanelContainer/BoxContainer/Back").Pressed += CloseMenu;
		GetNode<Button>("PanelContainer/BoxContainer/Exit")
			.Pressed += () => OnBtnPressed("Exit");
	}

	private void CloseMenu() => Visible = false;
	
	private void OnBtnPressed(string action)
	{
		switch (action)
		{
			case "Exit":
				_sceneTree.ChangeSceneToPacked(Loader.MainMenuScenePackedTree);
				break;
			case "Options":
				//TODO: open the options scene
				break;
		}
		CloseMenu();
	}

	public override void _Input(InputEvent @event)
	{
		if (!@event.IsActionPressed("exit") || !Visible) return;
		_sceneTree.Paused = false;
		Visible = false;
		CloseMenu();
	}
}