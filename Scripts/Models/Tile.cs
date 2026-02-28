using System;
using Godot;
using Godot.Collections;

namespace TheFourFabled.Scripts.Models;

public partial class Tile(Dictionary<GameResource, uint> innerGameResource, Vector2I position, Tile.TileType type) : Resource
{
    public Vector2I Position { get; init; } = position;
    public Dictionary<GameResource, uint> InnerGameResource { get; init; } = innerGameResource;

    public TileType Type { get; init; } = type;

    public BonusResource? BonusResource { get; private set; } = null; 

    public enum TileType
    {
        Sea,
        Land,
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
            TileType.Beach => 4,
            _ => 0 // Others tiles also have 0
        };
    
    public uint GetProduct() =>
        type switch
        {
            TileType.Sea => 0,
            TileType.Land => 2,
            TileType.Beach => 1,
            _ => 0 // Others tiles also have 0
        };
    
    public uint GetOre() =>
        type switch
        {
            TileType.Sea => BonusResource is Models.BonusResource.Iron or Models.BonusResource.Coal
                or Models.BonusResource.Gold
                ? 2
                : (uint)0,
            TileType.Land => BonusResource is Models.BonusResource.Iron or Models.BonusResource.Coal
                or Models.BonusResource.Gold
                ? 5
                : (uint)2,
            TileType.Tundra => 1,
            _ => 0
        };

    // Others tiles also have 0
    public uint GetWater() =>
        type switch
        {
            TileType.Beach => 5,
            TileType.Desert => 0,
            TileType.Sea or TileType.Land => 2,
            TileType.Show or TileType.Tundra => 1,
            _ => 0
        };

    public uint GetWood() =>
        type switch
        {
            TileType.Land => 5,
            TileType.Beach or TileType.Tundra => 4,
            _ => 0
        };
}