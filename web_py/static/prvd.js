var vueapp = null;
function prvdfunc( Vue ) {
    vueapp = new Vue({
        data: {
            maindata: {},
            curPrvd: ''
        },

        methods: {
            isKeyReadOnly: function(key){
                return !(key in {weight:1, enable:1})
            },

            onClickName: function(pname){
                this.curPrvd = pname;
            },

            onItemValChange: function(event, itmobj, itmKey){
                console.log("change to " + itmobj);
                let targeVal = parseInt(event.target.value);
                if (!targeVal) {
                    alert("invalid value " + targeVal);
                    return;
                }

                let paramObj = {
                    svrid: itmobj.svrid,
                    regname: itmobj.regname,
                    prvdid: itmobj.prvdid
                };
                paramObj[itmKey] = targeVal;
                let url = "/notify/provider";
                let self = this;
                this.$http.get(url, {params:paramObj}).then(function(res){
                    if (res.body instanceof Object) {
                        //self.maindata = res.body.data;
                    }else {
                        alert('修改' + itmKey + '执行失败：' + JSON.stringify(res.body));
                    }
                }, function(){
                    alert('修改' + itmKey + '请求' + url + '失败');
                })
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