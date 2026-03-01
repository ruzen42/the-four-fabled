using Godot;
using System;
using System.Collections.Generic;

public partial class Console : CanvasLayer
{
	private RichTextLabel _output = null!;
	private LineEdit _input = null!;

	private Dictionary<string, Action<string[]>> _commands = new();
	private List<string> _history = [];
	private int _historyIndex = -1;
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

	private void OnSent_OnClick()
	{
		OnCommandEntered(_input.Text); 
	}

	private void OnCommandEntered(string text)
	{
		Print("> " + text);

		_history.Add(text);
		_historyIndex = _history.Count;
		
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
	}

	public override void _Input(InputEvent @event)
	{
		if (@event is InputEventKey { Pressed: true } key && Visible)
		{
			switch (key.Keycode)
			{
				case Key.Up:
					HistoryUp();
					break;
				case Key.Down:
					HistoryDown();
					break;
			}
		}
		
		if (@event is InputEventKey { Pressed: true, Keycode: Key.Tab }) 
			Toggle(); 
	}
	
	private void HistoryUp()
	{
		if (_history.Count == 0)
			return;

		_historyIndex--;

		if (_historyIndex < 0)
			_historyIndex = 0;

		_input.Text = _history[_historyIndex];
		_input.CaretColumn = _input.Text.Length;
	}
	
	private void HistoryDown()
	{
		if (_history.Count == 0)
			return;

		_historyIndex++;

		if (_historyIndex >= _history.Count)
		{
			_historyIndex = _history.Count;
			_input.Clear();
			return;
		}

		_input.Text = _history[_historyIndex];
		_input.CaretColumn = _input.Text.Length;
	}
}
