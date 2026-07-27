using System;
using Godot;
using TheFourFabled.Scripts.AutoLoad;

namespace TheFourFabled.Scripts.UI.MainMenu;

public partial class OptionsMenu : Control
{
	[Export]
	private OptionButton _resolutionOption;
	[Export]
	private OptionButton _graphicsOption;
	[Export]
	private CheckButton _soundTurnButton;
	[Export]
	private HSlider _masterSlider;
	[Export]
	private CheckButton _autoSavesButton;
	[Export]
	private SpinBox _spinBoxFrequency;

	public void OnBackPressed()
	{
		var selectedText = _resolutionOption.GetItemText(_resolutionOption.Selected);
		var parts = selectedText.Split('x');
		if (parts.Length == 2 && int.TryParse(parts[0], out var width) && int.TryParse(parts[1], out var height))
		{
			Saves.Resolution = new Vector2I(width, height);
			Saves.SetResolution(Saves.Resolution);
		}

		Saves.Graphics = _getGraphicsMode(_graphicsOption);
		var normalizedVolume = (float)(_masterSlider.Value / 100.0);
		Saves.SetAudioVolumes(normalizedVolume, normalizedVolume, normalizedVolume);

		Saves.SaveGame();

		Visible = false;
	}

	private static Saves.GraphicsMode _getGraphicsMode(OptionButton graphicsButton)
	{
		return graphicsButton.Selected switch
		{
			3 => Saves.GraphicsMode.High,
			1 => Saves.GraphicsMode.Low,
			0 => Saves.GraphicsMode.Celeron,
			_ => Saves.GraphicsMode.Medium 
		};
	}
}
