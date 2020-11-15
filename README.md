# robotArm
Software for a 3D Printed Robot Arm
Written by Florin Tobler
more information: http://www.thingiverse.com/thing:1718984

This version has been extended for: 
# Robot Arm 3D Scanner

Samsung Galaxy S20+ has a 3d scanner app to scan small things.
In order to scan an object, the phone must be moved around the object in a circle.
What is better suited to do the job than a robotarm ?

This code fork uses a single button (attached to x_min ramps input) to control the 3d scan.

After startup, the robotarm is un-armed.

Move the Arm in the right position so that the App detects the object.
Press the button to arm, now the robotarm should stay in this position.
If everything is correct, start the 3d scan on the app and press the button again to start the rotation (clockwise)

If something goes wrong, press the button to interrupt the rotation. The next press will move the arm back in the original position.
To reset/unarm the robotarm, long press the button.
