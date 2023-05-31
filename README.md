# AmbientOcclusion

  This code is about ambient occlusion from GPU Gems 2 & 3.
  
  Please refer to [this](https://jeesunkim.com/projects/gpu-gems/ambient_occlusion/) to see the details.
  
![result_merged](https://github.com/emoy-kim/AmbientOcclusion/assets/17864157/7ddd4432-5cb6-40e5-bb0d-05c21e444977)

## Keyboard Commands
  * **1 key**: select dynamic ambient occlusion algorithm
  * **2 key**: select high quality ambient occlusion algorithm
  * **up key**: increase the number of passes
  * **down key**: decrease the number of passes
  * **r key**: toggle robustness when _high quality ambient occlusion algorithm is selected_
  * **e(+left shift) key**: increase(decrease) proximity tolerance when _high quality ambient occlusion algorithm is selected_
  * **d(+left shift) key**: increase(decrease) distance attenuation when _high quality ambient occlusion algorithm is selected_
  * **t(+left shift) key**: increase(decrease) triangle attenuation when _high quality ambient occlusion algorithm is selected_
  * **b key**: toggle bent normal activation when calculating light effects
  * **l key**: toggle light effects
  * **c key**: capture the current frame
  * **SPACE key**: pause rendering
  * **q/ESC key**: exit
