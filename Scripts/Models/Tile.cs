using Godot.Collections;

namespace TheFourFabled.Scripts.Models;

public class Tile 
{
	public int X { get; set; }
	public int Y { get; set; }
	public Dictionary<GameResource, uint> InnerGameResource { get; init; }
}
