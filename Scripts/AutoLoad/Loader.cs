using Godot;

namespace TheFourFabled.Scripts.AutoLoad;

public partial class Loader : Node
{
    public static PackedScene GameSceneTree { get; private set; }= null!;
    public static PackedScene MainMenuScenePackedTree { get; private set; } = null!;

    public override void _Ready()
    {
        GameSceneTree = GD.Load<PackedScene>("res://Scenes/main_game.tscn"); 
        MainMenuScenePackedTree = GD.Load<PackedScene>("res://Scenes/main_menu.tscn"); 
    }
}