function mainf( Vue ) {
  


var global_data = {
  hello: "21",
  active_idx: 0
}

// 2. 定义路由
const routes = [
  { path: '/main', component: { template: '#id_main' } },
  { path: '/conf', component: { template: '#id_conf' } },
  { path: '/log', component: { template: '#id_log' } },
  { path: '/about', component: { template: '#id_about' } }
]

// 3. 创建 router 实例，然后传 `routes` 配置
const router = new VueRouter({
  routes // (缩写) 相当于 routes: routes
})

// 4. 创建和挂载根实例。
const app = new Vue({
  data: global_data,
  methods: {
    nat_click: function(idx) {
      console.log("cliedk " + idx);
      active_idx = idx;
    },
    active_classset: function(idx) {
      if (typeof(active_idx) == "undefined") {
        return 0 === idx ? "active" : "";
      }
      return idx === active_idx ? "active" : "";
    }
  },
  router
}).$mount('#app')

}

mainf(Vue);