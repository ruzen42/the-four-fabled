using System;
using System.Collections.Generic;
using Godot;
using static TheFourFabled.Scripts.Models.CivilizationStats;

namespace TheFourFabled.Scripts.Models;

public partial class City : Building
{
    public enum CityLevel
    {
        Hamlet = 0,
        Village = 1,
        Town = 2,
        City = 3,
        Metropolis = 4
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
    [Export] public Vector2 Center { get; set; }

    [Export] public CivilizationType Civilization { get; private set; }
    public bool IsCapital => Type == CityType.Capital;

    public List<Tile> Tiles { get; set; } = [];

    public City(CivilizationType type, CityType cityType, Tile center, string? name = "Linux") : base(center.Position)
    {
        Civilization = type;
        Type = cityType;
        Center = center.Position;
        Tiles.Add(center);

        if (name is null)
        {
            Name = type switch
            {
                CivilizationType.Bashkortostan => "Bashkortostan's City",
                CivilizationType.SalavatOrder => "Salavat's City",
                CivilizationType.GoldOrda => "Kizak",
                CivilizationType.FireFlyClan => "Firefly's City",
                _ => "Some"
            };
        }
        else
        {
            Name = name;
        }
    }
}