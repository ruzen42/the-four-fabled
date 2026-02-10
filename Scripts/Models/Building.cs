using Godot;
using Godot.Collections;

namespace TheFourFabled.Scripts.Models;

public abstract partial class Building(Vector2 position) : Resource
{
    public Vector2 Position { get; } = position;
    public Dictionary<GameResource, uint> Production { get; } = new();
    public uint Population { get; set; } = 1;
}