using Godot;
using System;

public partial class OptionsMenu : Control
{
	[Signal]
    public delegate void BackPressedEventHandler();

	public override void _Ready()
	{
	}
	
	public void OnBackButton_OnClicked()
	{
		EmitSignal(SignalName.BackPressed);
	}

	// Called every frame. 'delta' is the elapsed time since the previous frame.
	public override void _Process(double delta)
	{
	}
}
