package com.fh2recomp.activity;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import com.fh2recomp.R;

/**
 * MainActivity — tela inicial: disclaimer legal, seleção da pasta de assets (SAF)
 * e configurações (escala de resolução, FPS alvo, backend gráfico, HUD touch).
 *
 * O app NÃO distribui assets do jogo. O usuário fornece sua cópia legal.
 */
public class MainActivity extends AppCompatActivity {

    private static final String PREFS = "fh2recomp_prefs";

    private SharedPreferences prefs;
    private TextView folderLabel;
    private Uri assetsUri;
    private boolean assetsIsIso = false;

    private final ActivityResultLauncher<Intent> folderPicker =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == Activity.RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        // Persiste a permissão na árvore selecionada (SAF).
                        // O mask do takePersistableUriPermission aceita APENAS
                        // READ|WRITE: incluir FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                        // lança IllegalArgumentException ("Requested flags 0x41, but
                        // only 0x3 are allowed") e fechava o app ao escolher a pasta.
                        final int takeFlags = result.getData().getFlags()
                                & (Intent.FLAG_GRANT_READ_URI_PERMISSION
                                   | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                        try {
                            getContentResolver().takePersistableUriPermission(uri, takeFlags);
                        } catch (SecurityException | IllegalArgumentException ignored) {
                        }
                        assetsUri = uri;
                        assetsIsIso = false;
                        prefs.edit().putString("assets_uri", uri.toString())
                                .putBoolean("assets_is_iso", false).apply();
                        folderLabel.setText(uri.getPath());
                    }
                }
            });

    /** Seleção de ISO do jogo (documento único) — leitura direta via fd,
     *  SEM copiar para o app; a árvore GDFX do disco é montada no native. */
    private final ActivityResultLauncher<Intent> isoPicker =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == Activity.RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        final int takeFlags = result.getData().getFlags()
                                & (Intent.FLAG_GRANT_READ_URI_PERMISSION
                                   | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                        try {
                            getContentResolver().takePersistableUriPermission(uri, takeFlags);
                        } catch (SecurityException | IllegalArgumentException ignored) {
                        }
                        assetsUri = uri;
                        assetsIsIso = true;
                        prefs.edit().putString("assets_uri", uri.toString())
                                .putBoolean("assets_is_iso", true).apply();
                        String name = uri.getLastPathSegment();
                        folderLabel.setText("ISO: " + (name != null ? name : uri.getPath()));
                    }
                }
            });

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        prefs = getSharedPreferences(PREFS, MODE_PRIVATE);

        TextView version = findViewById(R.id.version_label);
        version.setText("v0.1.0 — recompilação estática Xenon (PPC → C++ → ARM64)");

        folderLabel = findViewById(R.id.folder_label);
        String saved = prefs.getString("assets_uri", null);
        if (saved != null) {
            assetsUri = Uri.parse(saved);
            assetsIsIso = prefs.getBoolean("assets_is_iso", false);
            folderLabel.setText((assetsIsIso ? "ISO: " : "")
                    + (assetsUri.getPath() != null ? assetsUri.getPath() : saved));
        }

        Button btnFolder = findViewById(R.id.btn_select_folder);
        btnFolder.setOnClickListener(v -> openFolderPicker());

        Button btnIso = findViewById(R.id.btn_select_iso);
        if (btnIso != null) {
            btnIso.setOnClickListener(v -> openIsoPicker());
        }

        Button btnLaunch = findViewById(R.id.btn_launch);
        btnLaunch.setOnClickListener(v -> launchGame());

        bindSeek(R.id.seek_resolution, R.id.label_resolution, "resolution_scale", 75, v -> v + "%");
        bindSeek(R.id.seek_opacity, R.id.label_opacity, "hud_opacity", 70, v -> v + "%");
        bindSwitch(R.id.switch_fps, "fps_60", true);
        bindSwitch(R.id.switch_backend, "use_vulkan", true);
    }

    private void openFolderPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        folderPicker.launch(intent);
    }

    private void openIsoPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES,
                new String[]{"application/octet-stream", "application/x-iso9660-image"});
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        isoPicker.launch(intent);
    }

    private void launchGame() {
        if (assetsUri == null) {
            Toast.makeText(this, R.string.no_assets, Toast.LENGTH_LONG).show();
            return;
        }
        Intent intent = new Intent(this, GameActivity.class);
        intent.setData(assetsUri);
        intent.putExtra("assets_is_iso", assetsIsIso);
        intent.putExtra("resolution_scale", prefs.getInt("resolution_scale", 75));
        intent.putExtra("fps_60", prefs.getBoolean("fps_60", true));
        intent.putExtra("use_vulkan", prefs.getBoolean("use_vulkan", true));
        intent.putExtra("hud_opacity", prefs.getInt("hud_opacity", 70));
        startActivity(intent);
    }

    private void bindSeek(int seekId, int labelId, String key, int def, java.util.function.IntFunction<String> fmt) {
        SeekBar seek = findViewById(seekId);
        TextView label = findViewById(labelId);
        int val = prefs.getInt(key, def);
        seek.setProgress(val);
        label.setText(fmt.apply(val));
        seek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean fromUser) {
                label.setText(fmt.apply(p));
                prefs.edit().putInt(key, p).apply();
            }
            @Override public void onStartTrackingTouch(SeekBar s) { }
            @Override public void onStopTrackingTouch(SeekBar s) { }
        });
    }

    private void bindSwitch(int id, String key, boolean def) {
        Switch sw = findViewById(id);
        sw.setChecked(prefs.getBoolean(key, def));
        sw.setOnCheckedChangeListener((b, checked) -> prefs.edit().putBoolean(key, checked).apply());
    }
}
