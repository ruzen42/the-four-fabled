using Godot;

namespace TheFourFabled.Scripts.Models;

public class City : Building
{
    public enum CityLevel
    {
        Hamlet,
        Village,
        Town,
        City,
        Metropolis
    }

    public enum CityType
    {
        Province,
        Capital,
        MilitaryCamp 
    }

    [Export] public string Name { get; set; }
    [Export] public uint Power { get; set; }
    [Export] public CityLevel Level { get; set; }
    [Export] public CityType Type { get; set; }
    [Export] public bool IsCapital => Type == CityType.Capital;
    
    [Export] public Tile[] Tiles { get; set; }
}