using Godot;
using System;

public partial class NetworkMenu : Control
{
	public override void _Ready()
	{
	}
	
	public void _OnBackPressed()
	{
		Visible = false;
	}
}
