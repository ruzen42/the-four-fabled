using System.Collections.Generic;
using Godot;
using TheFourFabled.Scripts.Models;

namespace TheFourFabled.Scripts.WorldInteraction;

public partial class MapLoader : Node
{
    [Export] public TileMapLayer MyTileMap;

    private readonly Dictionary<Vector2I, Tile> _gameTiles = new();

    public MapLoader() => MyTileMap = GetParent<TileMapLayer>();

    public override void _Ready() => SyncLogicWithTileMap();

    public void SyncLogicWithTileMap()
    {
        var usedCells = MyTileMap.GetUsedCells();

        foreach (var coords in usedCells)
        {
            var data = MyTileMap.GetCellTileData(coords);

            //var typeId = (int)data.GetCustomData("tile_type_id");

            //var newTile = new Tile(new Godot.Collections.Dictionary<GameResource, uint>(), coords,
            //    (Tile.TileType)typeId);

            //InitializeDefaultResources(newTile);

            //_gameTiles[coords] = newTile;
        }
    }

    private static void InitializeDefaultResources(Tile tile)
    {
        switch (tile.Type)
        {
            case Tile.TileType.Land:
                tile.InnerGameResource[GameResource.Wood] = 10;
                tile.InnerGameResource[GameResource.Ore] = 2;
                break;
        }
    }
}