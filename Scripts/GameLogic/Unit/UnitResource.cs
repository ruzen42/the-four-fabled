using Godot;
namespace TheFourFabled.Scripts.GameLogic.Unit;


[GlobalClass] 
public partial class UnitResource : Resource
{
    [Export] public string Name { get; set; }
    [Export] public uint MaxHealth { get; set; }
    [Export] public uint Health { get; set; }
    [Export] public uint MaxMana { get; set; }
    [Export] public uint Mana { get; set; }
    [Export] public uint Level { get; set; }
    [Export] public uint Experience { get; set; }
    [Export] public uint MovementPoints { get; set; }
    [Export] public Texture2D Icon { get; set; }
    [Export] public uint AttackPower { get; set; }
}