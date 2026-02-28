using Godot;
using System;
using System.Collections.Generic;

public partial class Console : CanvasLayer
{
	private RichTextLabel _output = null!;
	private LineEdit _input = null!;

	private Dictionary<string, Action<string[]>> _commands = new();

	public override void _Ready()
	{
		_output = GetNode<RichTextLabel>("Panel/Output");
		_input = GetNode<LineEdit>("Panel/Input");

		_input.TextSubmitted += OnCommandEntered;

		Visible = false;

		RegisterCommands();
	}

	private void RegisterCommands()
	{
		_commands["help"] = args =>
		{
			Print("Available commands:");
			foreach (var cmd in _commands.Keys)
				Print(cmd);
		};

		_commands["clear"] = args =>
		{
			_output.Clear();
		};

		_commands["echo"] = args =>
		{
			Print(string.Join(" ", args));
		};
	}

	private void OnCommandEntered(string text)
	{
		Print("> " + text);

		ExecuteCommand(text);

		_input.Clear();
	}

	private void ExecuteCommand(string text)
	{
		var parts = text.Split(" ");
		var cmd = parts[0];
		var args = parts.Length > 1 ? parts[1..] : [];

		if (_commands.TryGetValue(cmd, out var value))
            value(args);
		else
			Print("Unknown command");
	}

	private void Print(string text) => _output.AppendText(text + "\n");

	private void Toggle()
	{
		Visible = !Visible;

		if (Visible)
			_input.GrabFocus();
	}
}