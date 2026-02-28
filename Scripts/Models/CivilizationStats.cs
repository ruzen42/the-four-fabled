using System.Collections.Generic;
using Godot;

namespace TheFourFabled.Scripts.Models;

public class CivilizationStats(CivilizationStats.CivilizationType type, City initialCity)
{
    public enum CivilizationType
    {
        Bashkortostan,
        SalavatOrder,
        GoldOrda,
        FireFlyClan
    }

    public CivilizationType Type { get; } = type;

    public List<IJob> Jobs = []; 

    public List<City> Cities { get; private set; } = [initialCity];
    
    public Dictionary<GameResource, uint> Resources { get; } = new();

    public void AddCity(City city)
    {
        Cities.Add(city);
        GD.Print($"City {city.Name} added to civilization {Type.ToString()}");
    }
}