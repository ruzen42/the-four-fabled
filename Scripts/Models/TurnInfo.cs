namespace TheFourFabled.Scripts.Models;

public enum TurnState { Waiting, Processing, Finished }

public class TurnInfo
{
    public int RoundNumber { get; set; } = 1;
    public CivilizationStats.CivilizationType CurrentCiv { get; set; }
    public TurnState State { get; set; } = TurnState.Waiting;
}