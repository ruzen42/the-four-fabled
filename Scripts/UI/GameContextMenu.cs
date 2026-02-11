using Godot;

namespace TheFourFabled.Scripts.UI;

public partial class GameContextMenu : Control
{
    [Signal]
    public delegate void ActionSelectedEventHandler(string actionName, Vector2I tileCoords);

    private Vector2I _currentTileCoords;
    private PanelContainer _menuPanel;

    public override void _Ready()
    {
        _menuPanel = GetNode<PanelContainer>("PanelContainer");

        Visible = false;

        GetNode<Button>("PanelContainer/VBoxContainer/BuildCity").Pressed += () => OnBtnPressed("BuildCity");
        GetNode<Button>("PanelContainer/VBoxContainer/BuildUpgrade").Pressed += () => OnBtnPressed("Upgrade");
    }

    public void ShowMenu(Vector2 screenPosition, Vector2I tileCoords)
    {
        _currentTileCoords = tileCoords;
        Position = screenPosition;
        Visible = true;
    }

    private void OnBtnPressed(string action)
    {
        EmitSignal(SignalName.ActionSelected, action, _currentTileCoords);
        CloseMenu();
    }

    public void CloseMenu() => Visible = false;

    public override void _UnhandledInput(InputEvent @event)
    {
        if (!Visible || @event is not InputEventMouseButton { Pressed: true } mouseBtn) return;

        var menuRect = _menuPanel.GetGlobalRect();

        if (!menuRect.HasPoint(mouseBtn.GlobalPosition))
        {
            CloseMenu();
        }
    }
}