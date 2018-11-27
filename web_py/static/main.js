function mainf( Vue ) {
  

const app = new Vue({
  data: {
    tdata: 123,
    maindata: [],
    colm_checked: ['svrid', 'svrname']
  },

  methods: {
    get_maindata : function(id) {
        this.$http.get('/clidata/' + id).then(function(res){
            var result = res.body.code
            if (0 != result) {
                alert('获取失败：'+res.body.desc)
            } else {
                this.maindata = res.body.data;
            }
        }, function(){
            alert('请求/svrall失败处理');
        })
    }

  },

  created: function() {
    this.get_maindata(0);
  }

  
}).$mount('#app')

}

mainf(Vue);