using System;
using Godot;
using Godot.Collections;

namespace TheFourFabled.Scripts.Models;

public partial class Tile(Dictionary<GameResource, uint> innerGameResource, Vector2I position, Tile.TileType type) : Resource
{
    public Vector2I Position { get; init; } = position;
    public Dictionary<GameResource, uint> InnerGameResource { get; init; } = innerGameResource;

    public TileType Type { get; init; } = type;

    public enum TileType
    {
        Sea,
        Land,
        Resource,
        Beach,
        Desert,
        Show,
        Tundra
    }

    public uint GetFood() =>
        type switch
        {
            TileType.Sea => 2,
            TileType.Land => 3,
            TileType.Resource => 2,
            TileType.Beach => 4,
            _ => 0 // Others tiles also have 0
        };
}