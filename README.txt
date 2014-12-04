WINDOWLACKEY
a light-based automatic window blinds controller

If you want to build one of these exactly like I did, you will require:
  1x Arduino Pro Mini 5V https://www.sparkfun.com/products/11113
  1x sunkee light sensor board http://www.amazon.com/gp/product/B00CW82WZY/
  1x zitrades motor driver board http://www.amazon.com/gp/product/B00E6NIMAM/
  1x tiny motors http://www.amazon.com/gp/product/B009AQLDSS/
  2x 607 bearings http://www.amazon.com/gp/product/B00K85I51I/
  1x SPST switch http://www.amazon.com/dp/B00AKVBEN6
  1x power supply http://www.amazon.com/gp/product/B00KC9Z5Y0/
  1x DC power jack http://www.amazon.com/dp/B00CQMGWIO/
  1x m7 x 35mm long hex head bolt
  2x flat-head wood screws, 3mm to 4mm diameter by about 15mm long
  1x pan-head wood screw, 3mm to 4mm diameter by 20-25mm long
  
A website like www.mouser.com sells similar switches and DC power jacks for much cheaper, but you have to pay shipping.

It also requires my BH1750FVI Arduino library https://github.com/drewtm/BH1750FVI

You'll also need an FTDI cable, or access to one temporarily, to program the Arduino.

You can 3D print the plastic parts from the included STL files if you have the exact same type of blinds that I have.

The wiring in that case is done in-place with small solid-core wire (i used wire from an ethernet cable),
and the wires lock all the circuit boards together into the case.

The gears and motor are for a ball-chain type blinds mechanism, often found on vertical blinds.