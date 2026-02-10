using System.Collections.Generic;
using Godot;

namespace TheFourFabled.Scripts.Models;

public class CivilizationStats(CivilizationStats.CivilizationType type)
{
    public enum CivilizationType
    {
        Bashkortostan,
        SalavatOrder,
        GoldOrda,
        FireFlyClan
    }

    public CivilizationType Type { get; } = type;
    
    public List<City> Cities { get; private set; }

    public void AddCity(City city)
    {
        Cities.Add(city);
        GD.Print($"City {city.Name} added to civilization {Type.ToString()}");
    }
}