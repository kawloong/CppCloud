function mainf( Vue ) {
  

const app = new Vue({
  data: {
    maindata: [],
    colm_checked: ['svrid', 'svrname', 'clisock'],
    sort_colm: "",
    sort_dir: 0 // 1 升序； -1 降序
  },

  methods: {
    onSort : function(column, isNum) {
        var dir = this.sort_colm === column ? -1*this.sort_dir : 1;
        this.maindata.sort(function(left, right){
            left = isNum? Number(left[column]): left[column];
            right = isNum? Number(right[column]): right[column];
            console.log("left="+left+" right="+right);
            var ret = (left < right) ? -1 : (left > right ? 1 : 0);
            return ret * dir;
        });

        this.sort_colm = column;
        this.sort_dir = dir;
        console.log(this.sort_colm, dir);
    },
    get_maindata : function(id) {
        this.$http.get('/clidata/' + id).then(function(res){
            var result = res.body.code
            if (0 != result) {
                alert('获取失败：'+res.body.desc);
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