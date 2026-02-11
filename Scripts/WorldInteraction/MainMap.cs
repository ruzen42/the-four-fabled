using Godot;
using TheFourFabled.Scripts.UI;

namespace TheFourFabled.Scripts.WorldInteraction;

public partial class MainMap : TileMapLayer
{
#pragma warning disable CS8618 // Non-nullable field must contain a non-null value when exiting constructor. Consider adding the 'required' modifier or declaring as nullable.
    [Export] public GameContextMenu ContextMenu;
#pragma warning restore CS8618 // Non-nullable field must contain a non-null value when exiting constructor. Consider adding the 'required' modifier or declaring as nullable.

    public override void _Input(InputEvent @event)
    {
        if (@event is not InputEventMouseButton { Pressed: true, ButtonIndex: MouseButton.Right }) return;
        var globalMousePos = GetGlobalMousePosition();

        var localPos = ToLocal(globalMousePos);

        var mapSelection = LocalToMap(localPos);

        OnTileClicked(mapSelection);
    }

    private void OnTileClicked(Vector2I coords)
    {
        var sourceId = GetCellSourceId(coords);

        if (sourceId == -1) 
        {
            GD.PrintErr($"Unregistered click: {coords}");
            return;
        }
        
        GD.Print($"Click Right {coords}");

        //var data = GetCellTileData(coords);

        var screenMousePos = GetViewport().GetMousePosition();
            
        ContextMenu.ShowMenu(screenMousePos, coords);
    }
}