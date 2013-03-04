package lry.FFmpegLry;

import java.io.File;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;

public class FFmpegLryActivity extends Activity {
    /** Called when the activity is first created. */
	
	ImageView imageView = null;
	Button btnFree = null;
	Button btnFrame = null;
	
	//全局变量
	private Bitmap mBitmap;
	private int mSecs = 0;
	
	//导入动态库
    static {
        System.loadLibrary("ffmpeg");
        }
    public native String GetFFmpegVersion();
    
    //声明动态库函数
    private static native void EnvConfigure();
	private static native void drawFrame(Bitmap bitmap);
	private static native void EnvFree();
    
    
    @Override    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        imageView = (ImageView)findViewById(R.id.frame);
        btnFrame = (Button)findViewById(R.id.frame_adv);
        btnFree = (Button)findViewById(R.id.btnFree);
        
        //输出测试
        System.out.println("Hello, ffmpeg~");
        
        //获取版本序列号，并打印
        System.out.println(GetFFmpegVersion()); 
        
        //创建bitmap对象
        mBitmap = Bitmap.createBitmap(320, 240, Bitmap.Config.ARGB_8888);
        
        //解码环境配置准备
        String filePath = Environment.getExternalStorageDirectory() + File.separator;
        filePath = filePath + "external_sdcard/video/352x288.264";  //平板目录
//        filePath = filePath + "video/352x288.264";
        System.out.println("path: " + filePath);
        EnvConfigure();
        
        //设置按钮刷心帧
        btnFrame.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				//更新显示图像
				drawFrame(mBitmap);				
				imageView.setImageBitmap(mBitmap);
			}
		});
        
        //设置按钮释放内存
        btnFree.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
//				EnvFree();
				btnFrame.setEnabled(false);
				btnFree.setEnabled(false);
			}
		});
        
    } 
    
    
    
}