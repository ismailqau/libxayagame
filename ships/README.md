# Xayaships

*Xayaships* is an example game on the Xaya platform, demonstrating how
[**game
channels**](http://www.ledgerjournal.org/ojs/index.php/ledger/article/view/15)
can be used for trustless and fully decentralised, scalable off-chain gaming.

This is a simple implementation of the classical game
[Battleships](https://en.wikipedia.org/wiki/Battleship_%28game%29), where
players compete against each other and scores are recorded in the
on-chain game state for eternity.

Note that Xayaships is mainly intended to be an example game for demonstrating
the capabilities of game channels, testing the implementation and releasing
code that can be used and extended for other games.  Nevertheless, it provides
a fully-functional game, that is still fun to play.

The [game ID](https://github.com/xaya/xaya/blob/master/doc/xaya/games.md#games)
of Xayaships is `g/xs`.

## Game Overview

The on-chain game state of Xayaships keeps track, for each Xaya username,
of how many games that user has won and lost.  It also stores the
currently open game channels as needed to process disputes.

Channels can be opened at any time by sending a move requesting to
open a channel, and anyone can send a move to join an existing channel
waiting for a second player.  Channels can be closed anytime by the
player in it as long as no other has joined.  If there are already
two players in a channel, then it can be closed by any player (typically
the winner) providing a message stating who won and signed by both participants.

The battleships game itself (on a channel) is played on an **8x8 board**.
(This allows us conveniently to store a boolean map of the entire board
in an unsigned 64-bit integer.)
The following ships are available for placement by each player:

- 1x ship of size 4
- 2x ship of size 3
- 4x ship of size 2

Ships can be placed anywhere horizontally or vertically, but are not
allowed to overlap and also have to have a distance of at least one
empty grid square (including diagonally) to other ships.

At the beginning of the game, each player places their ships secretly
and publishes a *hash* of their chosen position (and some salt to make
brute-force guessing impossible).  Together with that hash, they also publish
a second hash of some random data.  After that, both reveal the second data,
and the combination is used to randomly determine who starts the game.

Then the players take turns guessing a location, and the other player
responds whether or not a ship has been hit; for the moment, this
report is "trusted".
When a ship has been hit, the player can make another guess.  Otherwise
it becomes the opponent's turn.

(Note that in contrast to typical rules, there is no distinction between
"hit" and "sunk".  This simplifies the game, which helps build a good
and easy to understand example game.)

At any time, a player can end the game by publishing their full configuration
and salt.  (This is typically done if all opponent ships have been sunk or
if the player gets an impossible answer from the opponent.)  In that case,
the opponent also has to publish their configuration
and salt, and they are checked against hashes as well as the reported outcomes
of shots during the game.  If a player lied, he loses the game.  Otherwise,
the ending player wins if and only if all opponent ships have been sunk.

## Detailed Specification

Let us now define the game rules (moves for the on-chain state, i.e.
management of channels, and the board rules for games in a channel)
in more detail.

### Board State and Rules

The *board state* is the state of a particular game of battleships in
a channel.  The exact sequence of moves in such a game is as follows:

1. Alice opens a channel by sending an on-chain move.
1. Bob joins the channel by sending also an on-chain move.
1. Alice sends two hashes as move in the channel, one of her chosen
   ship configuration and one of her chosen random seed.
1. Bob sends the hash of his ship configuration and at the same time
   his random seed in clear text.
1. Alice reveals her random seed.
1. The concatenation of both random seeds is hashed and the result
   used to derive a random bit.  That determines who (Alice or Bob)
   starts the main game and is the next player.  For this example, let us
   say Alice is chosen.
1. Alice guesses a coordinate on the board for her "shot".
  - Besides verifying that the coordinate is valid (i.e. in range, well-formed),
    it is also verified that the coordinate has not been guessed before.
    If it has, then the move is invalid.
1. Bob responds with "hit" or "miss".
  - If it is "hit", then it stays Alice's turn.
  - If it is a "miss", then Bob is the next player.
1. Any time, both Alice and Bob can decide to end the game instead of
   either guessing the next coordinate or responding to a guess.  Let's say
   that Bob ends the game.
1. In that situation, Bob reveals his ship configuration and salt.
  - The move is valid as long as this preimage matches his committed hash.
  - If any of Bob's previous answers to Alice's guesses was wrong according to
    the configuration, then the state is marked as "Alice won".
  - Similarly, if Bob's configuration is invalid for the game (because
    he placed too few ships, they touch or anything like that),
    then Alice also wins.
1. If Bob did not lie anywhere and all of Alice's ships have been hit
    (according to Alice's answers), then the state is marked as "Bob won".
1. Otherwise, Alice reveals her configuration in the next move.
  - As before, the move is valid if and only if the preimage matches Alice's
    committed hash.
  - If any of Alice's answers was wrong or her initial position invalid,
    then the state is marked as "Bob won".
1. If also Alice did not lie at any time, then Alice wins (since Bob did not
   sink all her ships).
1. The player who *lost* creates a message containing the channel ID and
   that her opponent won, and signs it with her address.  This is the last
   move in the channel game.
1. The winning player can then close the channel and get the results marked
   on-chain by sending a move containing that signed message.

In a situation where both players have already sunk all ships, the rules
detailed above imply that the player who reveals the initial position first
wins the game (rather than e.g. the player who sunk all ships first).
However, when e.g. Alice hits the last of Bob's ships, it is her turn again
and she knows that Bob either lied or she sunk all ships.  Thus in that
situation, she can immediately end the game and is guaranteed to win
(unless she lied about her ships).  Hence the player who sinks all
enemy ships first can ensure they win the game.