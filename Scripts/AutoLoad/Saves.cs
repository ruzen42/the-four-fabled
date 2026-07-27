using Godot;
using System;

namespace TheFourFabled.Scripts.AutoLoad;

public partial class Saves : Node
{
	private static readonly ConfigFile Config = new();
	private const string PathToSaveOptions = "user://options.cfg";

	private const string AudioSection = "audio";
	private const string DisplaySection = "display";
	private const string StatisticsSection = "statistics";
	private const string GameSection = "game";

	private const string KeyMaster = "master";
	private const string KeyMusic = "music";
	private const string KeySound = "sound";
	private const string KeyFullscreen = "fullscreen";
	private const string KeyResolution = "resolution";
	private const string KeyGameOpen = "open";
	private const string KeySavesUsed = "saves";
	private const string KeyVictories = "wins";
	private const string KeyFreqAutoSave = "autosave";
	private const string KeyGraphics = "graphics";

	public enum GraphicsMode
	{
		High,
		Medium,
		Low,
		Celeron
	}

	private static GraphicsMode _stringToGraphicsMode(string input)
	{
		return input switch
		{
			"High" => GraphicsMode.High,
			"Low" => GraphicsMode.Low,
			"Celeron" => GraphicsMode.Celeron,
			_ => GraphicsMode.Medium
		};
	}
	
	private static string _graphicsModeToString(GraphicsMode input)
	{
		return input switch
		{
			GraphicsMode.High => "High",
			GraphicsMode.Low => "Low",
			GraphicsMode.Celeron => "Celeron",
			_ => "Medium" 
		};
	}
	
	public static int GameOpen { get; private set; }
	public static int SavesUsed { get; private set; }
	public static int Victories { get; private set; }
	
	public static GraphicsMode Graphics { get; set; } 

	public static bool FullScreen { get; set; }
	public static Vector2I Resolution { get; set; }

	private static float _masterVolume = 1.0f;
	private static float _musicVolume = 1.0f;
	private static float _soundVolume = 1.0f;

	public static int SpinBox = 21;

	public static float MasterVolume
	{
		get => _masterVolume;
		set => _masterVolume = Mathf.Clamp(value, 0.0f, 1.0f);
	}

	public static float MusicVolume
	{
		get => _musicVolume;
		set => _musicVolume = Mathf.Clamp(value, 0.0f, 1.0f);
	}

	public static float SoundVolume
	{
		get => _soundVolume;
		set => _soundVolume = Mathf.Clamp(value, 0.0f, 1.0f);
	}

	public static event Action? AudioSettingsChanged;
	public static event Action? DisplaySettingsChanged;
	public static event Action? GameSaved;

	public override void _Ready()
	{
		var error = Config.Load(PathToSaveOptions);

		if (error == Error.FileNotFound)
		{
			ResetToDefaults();
			SaveGame();
		}
		else if (error != Error.Ok)
		{
			ResetToDefaults();
		}
		else
		{
			LoadSave();
			GD.Print("Options successfully loaded^");
		}

		GameOpen++;
		SaveGame();
		GD.Print($"Game opens (times): {GameOpen}");

		ApplyDisplaySettings();
	}

	public static void ResetToDefaults()
	{
		GameOpen = 0;
		SavesUsed = 0;
		Victories = 0;
		FullScreen = false;
		Resolution = new Vector2I(1920, 1080);
		_masterVolume = 1.0f;
		_musicVolume = 1.0f;
		_soundVolume = 1.0f;
		SpinBox = 21;
		Graphics = GraphicsMode.Celeron;
	}

	public static void ResetAudio()
	{
		_masterVolume = 1.0f;
		_musicVolume = 1.0f;
		_soundVolume = 1.0f;
		AudioSettingsChanged?.Invoke();
	}

	public static void LoadSave()
	{
		GameOpen = Config.GetValue(StatisticsSection, KeyGameOpen, 0).AsInt32();
		SavesUsed = Config.GetValue(StatisticsSection, KeySavesUsed, 0).AsInt32();
		Victories = Config.GetValue(StatisticsSection, KeyVictories, 0).AsInt32();

		FullScreen = Config.GetValue(DisplaySection, KeyFullscreen, false).AsBool();
		Graphics = _stringToGraphicsMode(Config.GetValue(DisplaySection, KeyGraphics, "Medium").AsString());
		Resolution = Config.GetValue(DisplaySection, KeyResolution, new Vector2I(1920, 1080)).AsVector2I();

		_masterVolume = Config.GetValue(AudioSection, KeyMaster, 1.0f).AsSingle();
		_musicVolume = Config.GetValue(AudioSection, KeyMusic, 1.0f).AsSingle();
		_soundVolume = Config.GetValue(AudioSection, KeySound, 1.0f).AsSingle();

		SpinBox = Config.GetValue(GameSection, KeyFreqAutoSave, 21).AsInt32();
	}

	public static void SaveGame()
	{
		SavesUsed++;

		Config.SetValue(StatisticsSection, KeyGameOpen, GameOpen);
		Config.SetValue(StatisticsSection, KeySavesUsed, SavesUsed);
		Config.SetValue(StatisticsSection, KeyVictories, Victories);

		Config.SetValue(DisplaySection, KeyFullscreen, FullScreen);
		Config.SetValue(DisplaySection, KeyResolution, Resolution);
		Config.SetValue(DisplaySection, KeyGraphics, _graphicsModeToString(Graphics));

		Config.SetValue(AudioSection, KeyMaster, _masterVolume);
		Config.SetValue(AudioSection, KeyMusic, _musicVolume);
		Config.SetValue(AudioSection, KeySound, _soundVolume);
		Config.SetValue(GameSection, KeyFreqAutoSave, SpinBox);

		var error = Config.Save(PathToSaveOptions);

		if (error == Error.Ok)
		{
			GameSaved?.Invoke();
			GD.Print("Successfully saved^.");
		}
		else
		{
			GD.PrintErr($"Error while saving: {error}");
		}
	}

	public static void ApplyDisplaySettings()
	{
		if (FullScreen)
		{
			DisplayServer.WindowSetMode(DisplayServer.WindowMode.Fullscreen);
		}
		else
		{
			DisplayServer.WindowSetMode(DisplayServer.WindowMode.Windowed);
			DisplayServer.WindowSetSize(Resolution);
		}
		DisplaySettingsChanged?.Invoke();
	}

	public static void Win()
	{
		Victories++;
		SaveGame();
	}

	public static void SetResolution(Vector2I resolution)
	{
		Resolution = resolution;
		ApplyDisplaySettings();
		SaveGame();
	}

	public static void SetFullScreen(bool fullscreen)
	{
		FullScreen = fullscreen;
		ApplyDisplaySettings();
		SaveGame();
	}

	public static void SetAudioVolumes(float master, float music, float sound, bool save = true)
	{
		MasterVolume = master;
		MusicVolume = music;
		SoundVolume = sound;
		AudioSettingsChanged?.Invoke();
		
		if (save)
			SaveGame();
	}
}
