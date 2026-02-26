using Godot;
using System;

namespace TheFourFabled.Scripts.Network;

public partial class NetworkManager : Node
{
    ENetMultiplayerPeer peer;

    public void HostGame(int port = 7777)
    {
        peer = new ENetMultiplayerPeer();
        peer.CreateServer(port);

        Multiplayer.MultiplayerPeer = peer;

        GD.Print($"[Network] Server started on {port}");
    }
    
    public void JoinGame(string ip)
    {
        peer = new ENetMultiplayerPeer();
        peer.CreateClient(ip, 7777);

        Multiplayer.MultiplayerPeer = peer;

        GD.Print("[Network] Connected to server");
    }
}