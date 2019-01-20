# UTHubAdvertiser
UT4 server plugin. Push a complete HUB summary to a configurable URL endpoint, at desired interval.

## Installing

### Plugin
Download repository ZIP, extract, and paste the entire folder into :
```
UTServer/UnrealTournament/Plugins/
```
Make sure the result path looks like this :
```
UTServer/UnrealTournament/Plugins/UTHubAdvertiser-master/Binaries/Linux/libUE4Server-UTHubAdvertiser-Linux-Shipping.so
```

### Configuration
Via Game.ini :
```ini
[/Script/UTHubAdvertiser.UTHubAdvertiser]

; Endpoint to post (json) data to
URL="https://example.com/api/post"

; Interval between each post
Interval=30

; add any desired headers to the requests
HeaderKeys="x-example-header"
HeaderValues="example value"
```
