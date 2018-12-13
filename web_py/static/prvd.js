var vueapp = null;
function prvdfunc( Vue ) {
    vueapp = new Vue({
        data: {
            maindata: {},
            curPrvd: ''
        },

        methods: {
            onClickName: function(pname){
                this.curPrvd = pname;
            }
        },

        computed: {
            
        },

        created: function() {
            const url = "/svrall?regname=all";
            let self = this;
            this.$http.get(url).then(function(res){
                if (res.body instanceof Object) {
                    self.maindata = res.body.data;
                }else {
                    console.log('执行失败：' + JSON.stringify(res.body));
                }
            }, function(){
                console.error('请求' + url + '失败');
            })
        },

        filters : {
            toHumanTime : function(val) {
              var d = new Date(val * 1000); 
              var date = (d.getFullYear()) + "-" + 
                         (d.getMonth() + 1) + "-" +
                         (d.getDate()) + " " + 
                         (d.getHours()) + ":" + 
                         (d.getMinutes()) + ":" + 
                         (d.getSeconds());
              return date;
            }
        }
    }).$mount('#app')
};

prvdfunc(Vue);