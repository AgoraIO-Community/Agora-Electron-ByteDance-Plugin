##Plugin API

```
Step by step to use beauty plugin:
 
1: const engine = new AgoraRtcEngine()
   engine.initialize(config.appId)
   engine.initializePluginManager()

2: const libPath = isMac ? 
      path.resolve(__static, 'bytedance/libByteDancePlugin.dylib')
    : path.resolve(__static, 'bytedance/ByteDancePlugin.dll')
  
  this.rtcEngine.registerPlugin({
      id: 'bytedance',
      path: libPath
  })
  
3: plugin.setParameter(....) 

4: const plugin = getRtcEngine().getPlugins().find(plugin => plugin.id === 'bytedance')

  if(plugin) {
      plugin.enable()
  }

5: if(plugin) {
    plugin.disable()
  }
    
6: engine.releasePluginManager()
```


*****
### - initializePluginManager(): number;

 
- It will init plugin manager moudle for rtcEngine. 
- @note
- You need to call it after you initialize the rtcEngine.
- You need to call releasePluginManager() to release the memory when you do not want to use this manager anymore.
- @return 
- 0: call api success;
- < 0: call api fail;

```
samplecode:

const engine = new AgoraRtcEngine()
engine.initialize(config.appId)
engine.initializePluginManager()
```
*****
### - releasePluginManager(): number;


- It will help you to release the plugin manager,you can not call any api of plugin after you call releasePluginManager()
- @return
- 0: call api success;
- < 0: call api fail;

*****
### - registerPlugin(info: PluginInfo): number;


- You can registe a plugin into the rtcEngine.
- @note
- You need to call initializePluginManager() first and then call this api.

- @parameter
- PluginInfo {
-     id: string;
-     path: string;
- }

- id：It is the unique ID of this plugin，you can get this plugin by this id.
- path: The lib path of this plugin.
- @return
- 0: call api success
- <0: call api fail


```
samplecode:

const libPath = isMac ? 
    path.resolve(__static, 'bytedance/libByteDancePlugin.dylib')
  : path.resolve(__static, 'bytedance/ByteDancePlugin.dll')

this.rtcEngine.registerPlugin({
    id: 'bytedance',
    path: libPath
})
```
*****
### - unregisterPlugin(pluginId: string): number;


- You can unregiste a plugin by this api.
- @parameter
- pluginId: The unique ID of this plugin which you set by registerPlugin() before.
- @return
- 0: call api success
- <0: call api fail

*****
### - getPlugins(): Plugin[];

- You can get all plugin info which you registe by registerPlugin(info: PluginInfo).
- @return
- Plugin[]: Return a list which contain all of the Plugin Object which you registe before.

*****
### - setParameter: (param: string) => number;

- You can set beauty authdata and beauty by this api.
- @note
- You must call setParameter(param: string) to set authdata and beauty parameters.
- @return
- 0: call api success
- <0: call api fail

```
samplecode:
plugin.setParameter(JSON.stringify({
  "plugin.bytedance.licensePath": path.join(__static, "bytedance/resource/license.licbag")
}))

```

*****
### - enable: () => number;

- You can enable this plugin by this api, because the default settings is disable when you registe this Plugin.
- @note
- You need to call setParameter(param: string) to set the beauty param fisrt.
- @return 
- 0: call api success
- <0: call api fail


```
samplecode:

const plugin = getRtcEngine().getPlugins().find(plugin => plugin.id === 'bytedance')
if(plugin) {
  plugin.enable()
}
```

*****
### - disable: () => number;

- You can pause this plugin by this api and you can resume it by enable();
- @return
- 0: call api success
- <0: call api fail


```
samplecode:

const plugin = getRtcEngine().getPlugins().find(plugin => plugin.id === 'bytedance')
if(plugin) {
  plugin.disable()
}
```

