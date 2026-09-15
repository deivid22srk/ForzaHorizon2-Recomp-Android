package com.fh2recomp.nativebridge;

/**
 * Ponte JNI para o runtime nativo (libfh2recomp.so).
 *
 * O runtime nativo implementa: boot do código recompilado (PPC → C++),
 * backend gráfico (GLES 3.1 / Vulkan), áudio (AAudio) e entrada.
 */
public final class NativeBridge {

    static {
        System.loadLibrary("fh2recomp");
    }

    /**
     * Inicializa o runtime e inicia a thread principal do jogo.
     * @param assetsTreeUri URI SAF da pasta de assets fornecida pelo usuário
     * @return true se o boot do runtime iniciou com sucesso
     */
    public native boolean nativeBoot(String assetsTreeUri, int resolutionScalePct,
                                     boolean fps60, boolean useVulkan);

    /** Cria/realoca a superfície gráfica (chamado na UI thread via SurfaceHolder). */
    public native void nativeSetSurface(android.view.Surface surface, int width, int height);

    /** Ciclo de vida Android: pausado pelo sistema (perde foco, minimiza). */
    public native void nativePause();

    /** Ciclo de vida Android: retorna do pause (recria recursos perdidos se preciso). */
    public native void nativeResume();

    /** Encerramento definitivo. */
    public native void nativeStop();

    /** Diretório de arquivos do app (getFilesDir) — deve vir antes do boot. */
    public native void nativeSetFilesDir(String path);

    /**
     * Cópia lazy: copia o arquivo guestPath da árvore SAF persistida para o
     * app-specific storage (chamado PELO NATIVE quando um readFile falha).
     * @return true se o arquivo foi copiado e está legível em filesDir
     */
    public static boolean copyFromSaf(String guestPath) {
        android.content.Context ctx = appContext;
        if (ctx == null || assetsTreeUri == null) return false;
        try {
            android.net.Uri tree = android.net.Uri.parse(assetsTreeUri);
            android.net.Uri doc = android.provider.DocumentsContract.buildDocumentUriUsingTree(
                    tree, android.provider.DocumentsContract.getTreeDocumentId(tree));
            // caminho pode ter subdiretórios: procura recursivamente pelo nome
            return copyRecursive(ctx, doc, guestPath);
        } catch (Exception e) {
            android.util.Log.w(TAG, "copyFromSaf falhou: " + guestPath, e);
            return false;
        }
    }

    private static boolean copyRecursive(android.content.Context ctx, android.net.Uri dirDoc,
                                         String guestPath) throws Exception {
        String[] parts = guestPath.split("/");
        android.net.Uri current = dirDoc;
        // desce pelos diretórios intermediários (se existirem)
        for (int i = 0; i < parts.length - 1; i++) {
            android.net.Uri child = findChild(ctx, current, parts[i], true);
            if (child == null) return false;
            current = child;
        }
        android.net.Uri file = findChild(ctx, current, parts[parts.length - 1], false);
        if (file == null) return false;
        java.io.File out = new java.io.File(ctx.getFilesDir(), guestPath);
        java.io.File parent = out.getParentFile();
        if (parent != null && !parent.isDirectory()) parent.mkdirs();
        try (java.io.InputStream in = ctx.getContentResolver().openInputStream(file);
             java.io.OutputStream os = new java.io.FileOutputStream(out)) {
            byte[] buf = new byte[256 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
        }
        return out.length() > 0;
    }

    private static android.net.Uri findChild(android.content.Context ctx, android.net.Uri parent,
                                             String name, boolean isDir) throws Exception {
        android.net.Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(
                parent, android.provider.DocumentsContract.getDocumentId(parent));
        try (android.database.Cursor c = ctx.getContentResolver().query(children,
                new String[]{android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                             android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                             android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE},
                null, null, null)) {
            while (c != null && c.moveToNext()) {
                String displayName = c.getString(1);
                String mime = c.getString(2);
                boolean childIsDir = mime != null && mime.equals(android.provider.DocumentsContract.Document.MIME_TYPE_DIR);
                if (name.equals(displayName) && childIsDir == isDir) {
                    return android.provider.DocumentsContract.buildDocumentUriUsingTree(parent, c.getString(0));
                }
            }
        }
        return null;
    }

    private static android.content.Context appContext;
    private static String assetsTreeUri;
    private static final String TAG = "FH2/Bridge";

    /** Chamado pela GameActivity antes do boot. */
    public static void setAppContext(android.content.Context context, String safUri) {
        appContext = context.getApplicationContext();
        assetsTreeUri = safUri;
    }

    /** Botão virtual do HUD (códigos em TouchHudView.ButtonId). */
    public static native void onVirtualButton(int buttonId, boolean pressed);

    /** Analógico virtual do HUD (direção), valores normalizados [-1,1]. */
    public static native void onVirtualStick(float x, float y);

    /** Evento de gamepad físico (padrão Xbox 360). */
    public static native void onGamepadEvent(int controllerPort, int buttonCode, boolean pressed);

    /** Triggers analógicos e sticks de gamepad físico. */
    public static native void onGamepadAxis(int controllerPort, int axisCode, float value);
}
