using Godot;
using System;
using System.Collections.Generic;

public partial class Unit(string name, Unit.UnitType type, Vector2I pos) : Node2D
{
	public enum UnitType
	{
		Melee,
		Ranged,
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

	public Guid Id { get; init; } = Guid.NewGuid(); 
	
	public UnitType Type { get; init; } = type;

	public List<UnitEffect> Effects { get; private set; } = [];
	public string UnitName { get; private set; } = name;

	public Vector2I UnitPosition { get; private set; } = pos;

	public bool Move(Vector2I pos)
	{
		var trail = Math.Abs(Position.X - pos.X) + Math.Abs(Position.Y - pos.Y);
		if (trail == 0) return false;

		if (trail > CellsCanMove)
			return false;

		CellsCanMove -= (int)trail;
		Position = pos;

		return true;
	}

	public int Health { get; private set; } = 100;

	public int MaxHealth { get; private set; } = 100;
	
	public int Experience { get; private set; }

	public int Level => Experience / 5 + 1;
	
	public int Power { get; private set; } = 5;
	
	public int Defense { get; private set; } 
	
	public int CellsCanMove { get; private set; }

	public int CellsCanAttack => ComputeCellsCanAttack();
	
	public bool IsDead => Health <= 0;

	private int ComputeCellsCanAttack()
	{
		return Type switch
		{
			UnitType.Ranged => 2,
			UnitType.Magic => Level,
			UnitType.Support => Level / 2 + 1,
			_ => 1 
		};
	}
	
	public void GetDamage(int amount)
	{
		GD.Print($"[Game] Unit {Name} get`s damage {amount}, {Name} health now is {Health}/{MaxHealth}");
		var damageReduction = Defense / 100.0f;
		var damage = (int)Math.Ceiling(amount * (1.0f - damageReduction));
		Health -= damage;
		if (Health > 0) return;
		GD.Print($"[Game] Unit {Name} - {Id} dead");
		QueueFree();
	}

	public void Heal(int amount)
	{
		Health = Math.Min(Health + amount, MaxHealth);

		Effects.Remove(UnitEffect.Poisoned);

		GD.Print($"[Game] Unit {Name} healed for {amount}. Current health: {Health}/{MaxHealth}");
	}

	public void Poison()
	{
		GD.Print($"[Game] Unit {Name} poisoned");
		Effects.Add(UnitEffect.Poisoned);
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
			Effects.Remove(UnitEffect.Stunned);
		}
	}
}
