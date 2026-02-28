using System;
using System.Collections.Generic;
using Godot;

namespace TheFourFabled.Scripts.UI;

public partial class Dialog : Control
{
    [Export] private Label TitleLabel;
    [Export] private Label BodyLabel;
    [Export] private VBoxContainer ButtonsContainer;
    [Export] private Button ButtonPrefab; 

    public class DialogChoice
    {
        public string Text;
        public Action OnClick;
        public DialogChoice(string text, Action onClick)
        {
            Text = text;
            OnClick = onClick;
        }
    }

    public void Show(string title, string body, List<DialogChoice> choices)
    {
        TitleLabel.Text = title;
        BodyLabel.Text = body;

        foreach (var choice in choices)
        {
            var btn = ButtonPrefab.Duplicate() as Button;
            btn.Text = choice.Text;
            btn.Pressed += () => choice.OnClick.Invoke();
            ButtonsContainer.AddChild(btn);
        }

        Visible = true;
    }
}