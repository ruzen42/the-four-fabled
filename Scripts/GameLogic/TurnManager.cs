using TheFourFabled.Scripts.Models;
using System;
using System.Collections.Generic;
using System.Linq;
using Godot;

namespace TheFourFabled.Scripts.GameLogic;

public partial class TurnManager : Node
{
    private List<CivilizationStats> _participants = new();
    private int _currentCivIndex = 0;
    
    public CivilizationStats CurrentActiveCiv => _participants[_currentCivIndex];

    public event Action<CivilizationStats> OnTurnStarted;
    public event Action OnRoundEnded;

    public void Initialize(List<CivilizationStats> civs)
    {
        _participants = civs;
        StartTurn();
    }

    public void RequestEndTurn(CivilizationStats.CivilizationType civType)
    {
        if (civType != CurrentActiveCiv.Type) return;

        GD.Print($"Civ {civType} finished their turn.");
        EndTurn();
    }

    private void EndTurn()
    {
        _currentCivIndex++;

        if (_currentCivIndex >= _participants.Count)
        {
            _currentCivIndex = 0;
            ProcessEndOfRound(); 
        }

        StartTurn();
    }

    private void StartTurn()
    {
        GD.Print($"It's now {CurrentActiveCiv.Type}'s turn.");
        
        OnTurnStarted.Invoke(CurrentActiveCiv);
    }

    private void ProcessEndOfRound()
    {
        GD.Print("All civilizations acted. Processing global changes...");
        
        foreach (var civ in _participants)
        {
            foreach (var city in civ.Cities)
            {
                var totalFood    = (uint)city.Tiles.Sum(t => t.GetFood());
                var totalProduct = (uint)city.Tiles.Sum(t => t.GetProduct());
                var totalWater   = (uint)city.Tiles.Sum(t => t.GetWater());
                var totalOre     = (uint)city.Tiles.Sum(t => t.GetOre());
                var totalWood    = (uint)city.Tiles.Sum(t => t.GetWood());
                civ.Resources[GameResource.Food] += totalFood;
                civ.Resources[GameResource.Ore] += totalFood;
                civ.Resources[GameResource.Water] += totalWater;
                civ.Resources[GameResource.Ore] += totalOre;
                civ.Resources[GameResource.Wood] += totalOre;
            }
        }
        
        OnRoundEnded.Invoke();
    }
}