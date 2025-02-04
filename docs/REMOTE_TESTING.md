# Remote testing access

## Step 1: Install `cloudflared`

Installation instructions here: https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/downloads/

## Step 2: Proxy ssh command through cloudflared

Append the following configurations to your `~/.ssh/config` file:

### Linux

```
Host sssi.yaoyiheng.com
  ProxyCommand /usr/local/bin/cloudflared access ssh --hostname %h
```

### MacOS

```
Host sssi.yaoyiheng.com
  ProxyCommand /opt/homebrew/opt/cloudflared/bin/cloudflared access ssh --hostname %h
```

## Step 3: Connect to remote server

```
ssh user@sssi.yaoyiheng.com
```

# Remote flashing of new builds

*Prerequisite: .uf2 file compiled*

## Step 1: Deploy new binary to server

```
scp <name>.uf2 user@sssi.yaoyiheng.com:/home/common
```

## Step 2: ssh into remote server using instructions above

## Step 3: Execute picotool to flash new binary

```
picotool load <name>.uf2 -f
```

## Step 4: Monitor logs from UART

```
ls /dev/serial/by-id
cat /dev/serial/by-id/<specific_device>
```

