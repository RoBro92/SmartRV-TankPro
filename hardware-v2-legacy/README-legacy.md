 [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/O5O4AKQ37)

# SmartRV Water Tank Module
**A ESP based water tank module for a Camper/RV**

All feature requests are tracked in the GitHub issue tracker. 
[Feature Requests](https://github.com/RoBro92/feature-requests/issues)

This water tank module is designed to fill a gap in the market for accessible smart water tank monitoring. Other systems such as Pico and Victron do exist however their cost can be prohibitive and require additional components. This module is based on the ESP8266/32 by Espressif & is designed to work standalone or as part of a network to provide accurate and real-time updates for water tank level and temperature. It also provides a relay designed to interface with motorized ball valves or 2/3 wire and will allow remote draining of tanks without having to leave the vehicle. This not only makes filling the tank easier but can also provide emergency frost protection. 

The beauty of the ESP32 is that it can be easily programmed and by having the extra pins on a header you can add your own inputs/outputs to interface with the board. 

The module has been kept as compact as possible and allows for a direct connection to 12v or 24v vehicle systems. 


**If you want to support the development you can purchase the V1 board as a DIY kit or fully assembled V2  from my Ko-Fi Shop [SmartRV Shop](https://ko-fi.com/smartrv/shop)**

## **I have developed two separate versions of this module**

[V1 is THT and is suitable for at-home soldering](#tht)

[V2 is the final retail product and comes pre assembled inclduing software to allow easy OTA upload](#smd)





### THT

# **V1.3**
This release represents the final revision any further changes should be requested in the [Feature Requests](https://github.com/RoBro92/feature-requests/issues) section. It has been updated to include the design schematic and PCB layout. I will stock a number of these boards to be assembled by yourself and can also include supporting documentation. This device can also be supplied assembled if required. 

# **Changes**
- [x] - Updated to V1.3 final. 

# **Features**
- [x] - Removeable WemosD1 chip (ESP8266)
- [x] - Onboard power regulation
- [x] - Variable voltage input 8v-36v
- [x] - Compact form factor
- [x] - DS18b20 temperature sensor
- [x] - Resistive water level sensor
- [x] - 10A relay for water valve control
- [x] - LED Indicator for Power-ON
- [x] - Added reverse polarity protection
- [x] - Added LED indicators for power and relay
- [x] - Updated repository with Gerber production files



# **Images**
<table>
  <tr>
    <td>3D Model</td>
  </tr>
  <tr>
    <td><img src="V1%20THT/images/3dmodelfront.jpg" height=480></td>
  </tr>
 
 </table>
<table>
  <tr>
         <td>Schematic</td>
     <td>PCB Layout</td>

  </tr>
  <tr>
   <td><img src="V1%20THT/images/schematic.png" height=480></td>
   <td><img src="V1%20THT/images/pcblayout.png" height=480></td>    
  </tr>
 </table>

<table>
  <tr>
    <td>PCB Front v1.3</td>
     <td>PCB Rear v1.3</td>
  </tr>
  <tr>
    <td><img src="V1%20THT/images/pcbfront.jpg" height=480></td>
    <td><img src="V1%20THT/images/pcbrear.jpg" height=480></td>
  </tr>
 </table>

# **WARNING**
This board does not have any fuse protection as such this must be provided externally.

___

### SMD

# **V2.1**
This version is the final retail production of the SmartRV Water Tank Module. It comes with many improvements over the V1 version. 
Built primarily with surface mounted components this version is able to be as compact as possible while also retaining a full feature set. 
This board will be available to buy from my Kofi Shop.
 

It will also feature an ESP32 Module for several reasons;
- Bluetooth connection with AIO board to act as an additional wireless tank sensor
- Lower power consumption if being used with a battery
- Newer technology

# **Changes This Version**
- [x] - Added 3D printed case

# **Features**
- [x] - ESP32-C3-Mini
- [x] - Onboard power regulation
- [x] - Variable voltage input 8v-36v
- [x] - Compact form factor
- [x] - DS18b20 temperature sensor input
- [x] - Resistive water level sensor input with variable resistance selection via THT resistor
- [x] - 10A relay for water valve control 
- [x] - Reverse polarity protection
- [x] - External display provided via TX/RX for Nextion display
- [x] - Reverse polarity protection
- [x] - Expansion header for user customisation
- [x] - Bluetooth communication built into ESP32 module



# **Planned Changes**

- [ ] - Bluetooth app
- [ ] - Onboard USB programmer
- [ ] - Manual Override switch



# **Images**
<table>
  <tr>
    <td>Schematic</td>
     <td>PCB Layout</td>
  </tr>
  <tr>
    <td><img src="V2%20SMD/Images/schematic.png" height=480></td>
    <td><img src="V2%20SMD/Images/pcblayout.png" height=480></td>
  </tr>
 </table>

<table>
  <tr>
    <td>3D Model Front</td>
     <td>3D Model Rear</td>
  </tr>
  <tr>
    <td><img src="V2%20SMD/Images/3dfront.png" height=480></td>
    <td><img src="V2%20SMD/Images/3dback.png" height=480></td>
  </tr>
 </table>

<table>
  <tr>
    <td>Case</td>
     <td>Case Inside</td>
  </tr>
  <tr>
    <td><img src="V2%20SMD/Images/case.png" height=480></td>
    <td><img src="V2%20SMD/Images/caseinside.png" height=480></td>
  </tr>
 </table>

# **WARNING**
This board does not have any fuse protection as such this must be provided externally.

