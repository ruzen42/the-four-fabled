using Godot;
using System;

public partial class Tile 
{
	public int X { get; set; }
	public int Y { get; set; }
	public Resources innerResources { get; init; }
}
