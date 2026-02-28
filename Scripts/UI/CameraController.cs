using Godot;

namespace TheFourFabled.Scripts.UI;

public partial class CameraController : Camera2D
{
    [Export] public float DragSensitivity = 1.0f;
    [Export] public Vector2 ZoomMin = new(0.5f, 0.5f); 
    [Export] public Vector2 ZoomMax = new(4.0f, 4.0f); 
    [Export] public float ZoomSpeed = 0.1f;

    private bool _isDragging;

    public override void _Input(InputEvent @event)
    {
        switch (@event)
        {
            case InputEventMouseButton mouseButton:
            {
                switch (mouseButton.ButtonIndex)
                {
                    case MouseButton.Left:
                        _isDragging = mouseButton.Pressed;
                        break;
                    case MouseButton.WheelUp:
                        ApplyZoom(1 + ZoomSpeed);
                        break;
                    case MouseButton.WheelDown:
                        ApplyZoom(1 - ZoomSpeed);
                        break;
                }

                break;
            }
            case InputEventMouseMotion mouseMotion when _isDragging:
                Position -= mouseMotion.Relative * DragSensitivity / Zoom;
                break;
        }
    }

    private void ApplyZoom(float factor)
    {
        var newZoom = Zoom * factor;
        Zoom = newZoom.Clamp(ZoomMin, ZoomMax);
    }
}