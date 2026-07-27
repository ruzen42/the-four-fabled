using Godot;
using System;
using System.Collections.Generic;

public partial class Unit : Node2D
{
	public enum UnitType
	{
		Cavalry,
		AntiCavalry,
		Magic,
		Naval,
		Support
	}

	public enum UnitEffect
	{
		Poisoned,
		Healed,
		Stunned,
	}
	
	public UnitType Type { get; init; }
	
	public List<UnitEffect> Effects { get; private set; }
	public string Name { get; private set; }
	
	public Vector2I Position { get; private set; }

	public void Move(Vector2I pos)
	{
		Position = pos;	
	}

	public uint Health { get; private set; } = 100;

	public uint MaxHealth { get; private set; } = 100;
	
	public uint Experience { get; private set; }
	
	public uint Power { get; private set; } = 5;
	
	public uint Defense { get; private set; } 
	
	public uint CellsCanMove { get; private set; }
	
	public bool IsDead => Health <= 0;
	public override void _Ready()
	{
		Name = Type.ToString();
	}

	public void GetDamage(uint amount)
	{
		GD.Print($"[Game] Unit {Name} get`s damage {amount}, {Name} health now is {Health}/{MaxHealth}");
		Health -= amount * (Defense / 100);
		if (Health <= 0)
			GD.Print($"[Game] Unit {Name} dead");
	}

	public void Heal(uint amount)
	{
		GD.Print($"[Game] Unit {Name} heal {amount}, {Name} health now is {Health}/{MaxHealth}");
		Health += amount;
	}

	public override void _Process(double delta)
	{
	}

	public void PerTurn()
	{
		if (Effects.Contains(UnitEffect.Poisoned))
		{
			Health -= MaxHealth / 100 * 5;
		}
		else if (Effects.Contains(UnitEffect.Healed))
		{
			Health += MaxHealth / 100 * 5;
		}
		else if (Effects.Contains(UnitEffect.Stunned))
		{
			CellsCanMove = 0;
		}
	}
}
