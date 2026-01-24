PushPuzzle is a simple game, move the block dot (the pusher)
with the arrow keys to push the white blocks into
the white squares.  Undo can be done with 'a', exit with 'd'

To run you'll need the classes for the PushPuzzle, plus those for the
MID, and the pushpuzzle.zip file.  It contains the different levels
of the game.

PushPuzzle uses MIDletMain to launch.


WS=<MIDP workspace>
CP="$WS/build/solaris/classes:$WS/src/example/pushpuzzle/pushpuzzle.zip"
java -classpath $CP javax.microedition.midlet.MIDletMain example.pushpuzzle.PushPuzzle

